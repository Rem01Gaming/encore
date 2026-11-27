/*
 * Copyright (C) 2024-2026 Rem01Gaming
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "BinderMonitor.hpp"
#include "BinderNDK.hpp"
#include "Encore.hpp"
#include "EncoreLog.hpp"
#include <ShellUtility.hpp>

#include <cstring>
#include <cerrno>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <atomic>
#include <chrono>

// =============================================================================
// Transaction Codes Enum & Resolver Metadata
// =============================================================================

enum class TxCode {
    IsPowerSaveMode,
    IsInteractive,
    GetZenMode,
    RegisterProcessObserver,
    RegisterDisplayCallback,
    OnForegroundActivitiesChanged,
    OnForegroundServicesChanged,
    OnProcessDied,
    OnProcessStarted,
    OnDisplayEvent,
    GetPackageNameForUid
};

struct ResolverQuery {
    const char *query;
    bool isRequired;
    uint32_t fallback;
};

static const std::unordered_map<TxCode, ResolverQuery> kResolverQueries = {
        {TxCode::IsPowerSaveMode, {"android.os.IPowerManager.Stub::TRANSACTION_isPowerSaveMode", true, 0}},
        {TxCode::IsInteractive, {"android.os.IPowerManager.Stub::TRANSACTION_isInteractive", true, 0}},
        {TxCode::GetZenMode, {"android.app.INotificationManager.Stub::TRANSACTION_getZenMode", true, 0}},
        {TxCode::RegisterProcessObserver, {"android.app.IActivityManager.Stub::TRANSACTION_registerProcessObserver", true, 0}},
        {TxCode::RegisterDisplayCallback, {"android.hardware.display.IDisplayManager.Stub::TRANSACTION_registerCallback", true, 0}},
        {TxCode::OnForegroundActivitiesChanged, {"android.app.IProcessObserver.Stub::TRANSACTION_onForegroundActivitiesChanged", true, 0}},
        {TxCode::OnForegroundServicesChanged, {"android.app.IProcessObserver.Stub::TRANSACTION_onForegroundServicesChanged", true, 0}},
        {TxCode::OnProcessDied, {"android.app.IProcessObserver.Stub::TRANSACTION_onProcessDied", true, 0}},
        {TxCode::OnProcessStarted, {"android.app.IProcessObserver.Stub::TRANSACTION_onProcessStarted", false, 0}},
        {TxCode::OnDisplayEvent, {"android.hardware.display.IDisplayManagerCallback.Stub::TRANSACTION_onDisplayEvent", false, 1}},
        {TxCode::GetPackageNameForUid, { "android.content.pm.IPackageManager.Stub::TRANSACTION_getNameForUid", true, 0}},
};

// =============================================================================
// Internal state
// =============================================================================

struct BinderMonitorState {
    AIBinder *powerBinder = nullptr;
    AIBinder *notificationBinder = nullptr;
    AIBinder *activityBinder = nullptr;
    AIBinder *displayBinder = nullptr;
    AIBinder *packageBinder = nullptr;

    // Held alive so the remote services keep strong refs to our callbacks.
    AIBinder *processObserverBinder = nullptr;
    AIBinder *displayCallbackBinder = nullptr;

    std::unordered_map<TxCode, uint32_t> txCodes;

    ProcessObserverCallbacks processCallbacks;
    DisplayStateCallback displayCallback;
    PowerSaveCallback powerSaveCallback;

    bool displayLastState = false;
    bool powerSaveLastState = false;
    bool isMiui = false;

    std::thread powerSavePollThread;
    std::atomic<bool> stopPowerSavePolling{false};

    uint32_t getCode(TxCode code) const {
        auto it = txCodes.find(code);
        return it != txCodes.end() ? it->second : 0;
    }
};

static BinderMonitorState gState;

// =============================================================================
// Runtime transaction code resolver
// =============================================================================

static constexpr char kResolverSocketName[] = "encore_binder_resolver";

static bool sendResolverRequest(int socketFd, const std::string &request) {
    size_t sent = 0;
    while (sent < request.size()) {
        ssize_t result = send(socketFd, request.data() + sent, request.size() - sent, MSG_NOSIGNAL);
        if (result < 0 && errno == EINTR) continue;
        if (result <= 0) return false;
        sent += static_cast<size_t>(result);
    }
    return true;
}

static bool readResolverReply(int socketFd, std::string *reply) {
    reply->clear();
    char character;
    while (true) {
        ssize_t received = recv(socketFd, &character, sizeof(character), 0);
        if (received < 0 && errno == EINTR) continue;
        if (received <= 0) return false;
        if (character == '\n') return true;
        reply->push_back(character);
    }
}

static std::unordered_map<std::string, uint32_t> runResolver() {
    std::unordered_map<std::string, uint32_t> result;

    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    address.sun_path[0] = '\0';
    std::memcpy(address.sun_path + 1, kResolverSocketName, sizeof(kResolverSocketName) - 1);
    const socklen_t addressLength = static_cast<socklen_t>(
            offsetof(sockaddr_un, sun_path) + 1 + sizeof(kResolverSocketName) - 1);

    int socketFd = -1;
    int connectError = 0;
    for (int attempt = 0; attempt < 20; ++attempt) {
        socketFd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (socketFd < 0) {
            connectError = errno;
            break;
        }
        if (connect(socketFd, reinterpret_cast<sockaddr *>(&address), addressLength) == 0) {
            break;
        }
        connectError = errno;
        close(socketFd);
        socketFd = -1;
        if (connectError != ENOENT && connectError != ECONNREFUSED && connectError != EINTR) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    if (socketFd < 0) {
        LOGE_TAG("BinderMonitor", "Failed to connect to Zygisk resolver: {}", strerror(connectError));
        return result;
    }

    for (const auto &[code, query] : kResolverQueries) {
        const std::string request = std::string(query.query) + "\n";
        if (!sendResolverRequest(socketFd, request)) {
            LOGE_TAG("BinderMonitor", "Failed to send resolver query: {}", query.query);
            break;
        }

        std::string reply;
        if (!readResolverReply(socketFd, &reply)) {
            LOGE_TAG("BinderMonitor", "Failed to read resolver reply: {}", query.query);
            break;
        }
        if (reply.rfind("ERROR ", 0) == 0) {
            LOGW_TAG("BinderMonitor", "Resolver failed for {}: {}", query.query, reply);
            continue;
        }

        try {
            size_t parsed = 0;
            unsigned long value = std::stoul(reply, &parsed);
            if (parsed != reply.size() || value > UINT32_MAX) {
                throw std::out_of_range("invalid transaction code");
            }
            result[query.query] = static_cast<uint32_t>(value);
        } catch (...) {
            LOGW_TAG("BinderMonitor", "Invalid resolver reply for {}: {}", query.query, reply);
        }
    }

    close(socketFd);
    return result;
}

// =============================================================================
// Binder helpers
// =============================================================================

static void *noopCreate(void *args) { return args; }

static void noopDestroy(void *) {}

static binder_status_t noopTransact(AIBinder *, uint32_t, const AParcel *, AParcel *) {
    return STATUS_UNKNOWN_ERROR;
}

static bool stringAllocator(void *data, int32_t size, char **out) {
    if (size < 0) return false;
    auto *str = static_cast<std::string *>(data);
    str->resize(static_cast<size_t>(size));
    *out = str->data();
    return true;
}

/**
 * @brief Executes a binder transaction and reads back a single int32 reply value.
 */
static int32_t transactReadInt32(AIBinder *binder, uint32_t tx, const char *ifToken, int32_t onError = -1) {
    AParcel *in = nullptr, *out = nullptr;
    if (AIBinder_prepareTransaction(binder, &in) != STATUS_OK) {
        LOGE_TAG("BinderMonitor", "prepareTransaction failed for tx={} iface={}", tx, ifToken);
        return onError;
    }
    AParcel_writeInterfaceToken(in, ifToken);

    binder_status_t status = AIBinder_transact(binder, tx, &in, &out, 0);
    if (status != STATUS_OK) {
        LOGE_TAG("BinderMonitor", "transact failed for tx={} iface={} status={}", tx, ifToken, status);
        if (out) AParcel_delete(out);
        return onError;
    }

    AStatus *st = nullptr;
    AParcel_readStatusHeader(out, &st);
    bool ok = st && AStatus_isOk(st);
    if (!ok) {
        const char *desc = st ? AStatus_getDescription(st) : "null status";
        LOGE_TAG("BinderMonitor", "Reply status error for tx={} iface={}: {}", tx, ifToken, desc);
        if (st) {
            AStatus_deleteDescription(desc);
            AStatus_delete(st);
        }
        AParcel_delete(out);
        return onError;
    }
    if (st) AStatus_delete(st);

    int32_t value = onError;
    AParcel_readInt32(out, &value);
    AParcel_delete(out);
    return value;
}

/**
 * @brief Executes a binder transaction with a single int32 argument and reads back a single string reply value.
 */
static std::string transactReadString(AIBinder *binder, uint32_t tx, const char *ifToken, int32_t arg, const std::string &onError = "") {
    AParcel *in = nullptr, *out = nullptr;
    if (AIBinder_prepareTransaction(binder, &in) != STATUS_OK) {
        LOGE_TAG("BinderMonitor", "prepareTransaction failed for tx={} iface={}", tx, ifToken);
        return onError;
    }

    AParcel_writeInterfaceToken(in, ifToken);
    AParcel_writeInt32(in, arg);
    binder_status_t status = AIBinder_transact(binder, tx, &in, &out, 0);

    if (status != STATUS_OK) {
        LOGE_TAG("BinderMonitor", "transact failed for tx={} iface={} status={}", tx, ifToken, status);
        if (out) AParcel_delete(out);
        return onError;
    }

    AStatus *st = nullptr;
    AParcel_readStatusHeader(out, &st);
    bool ok = st && AStatus_isOk(st);

    if (!ok) {
        const char *desc = st ? AStatus_getDescription(st) : "null status";
        LOGE_TAG("BinderMonitor", "Reply status error for tx={} iface={}: {}", tx, ifToken, desc);
        if (st) {
            AStatus_deleteDescription(desc);
            AStatus_delete(st);
        }
        AParcel_delete(out);
        return onError;
    }
    if (st) AStatus_delete(st);

    std::string value;
    AParcel_readString(out, &value, stringAllocator);
    AParcel_delete(out);
    return value;
}

/**
 * @brief Waits for a service to register.
 */
static AIBinder *waitForServiceCompat(const char *name, int timeoutMs = 10000) {
    if (BinderNDK_hasSymbol("AServiceManager_waitForService")) {
        return AServiceManager_waitForService(name);
    }

    const int intervalMs = 100;
    int waited = 0;
    while (waited < timeoutMs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
        waited += intervalMs;
        AIBinder *binder = AServiceManager_getService(name);
        if (binder) return binder;
    }
    return nullptr;
}

static bool queryIsInteractive() {
    return transactReadInt32(gState.powerBinder, gState.getCode(TxCode::IsInteractive), "android.os.IPowerManager", 0) != 0;
}

static bool queryIsPowerSaveMode() {
    if (!gState.isMiui) {
        // This will return false everytime on MIUI/HOS
        return transactReadInt32(gState.powerBinder, gState.getCode(TxCode::IsPowerSaveMode), "android.os.IPowerManager", 0) != 0;
    }

    // Thanks Ximi for this fuckery
    PipeResult result = popen_direct({"/system/bin/settings", "get", "system", "POWER_SAVE_MODE_OPEN"});
    if (!result.stream) {
        LOGW_TAG("BinderMonitor", "Failed to read MIUI battery saver setting");
        return false;
    }

    char value[32] = {};
    if (!fgets(value, sizeof(value), result.stream)) return false;
    return value[0] == '1' && (value[1] == '\0' || value[1] == '\n' || value[1] == '\r');
}

// =============================================================================
// Binder onTransact callbacks
// =============================================================================

static binder_status_t processObserver_transact(AIBinder *, uint32_t code,
                                                const AParcel *in, AParcel *) {
    if (code == gState.getCode(TxCode::OnForegroundActivitiesChanged)) {
        int32_t pid = -1, uid = -1;
        bool foreground = false;
        AParcel_readInt32(in, &pid);
        AParcel_readInt32(in, &uid);
        AParcel_readBool(in, &foreground);
        if (gState.processCallbacks.onForegroundActivitiesChanged) {
            gState.processCallbacks.onForegroundActivitiesChanged(pid, uid, foreground);
        }
        return STATUS_OK;
    }
    if (code == gState.getCode(TxCode::OnForegroundServicesChanged)) {
        int32_t pid = -1, uid = -1, serviceTypes = 0;
        AParcel_readInt32(in, &pid);
        AParcel_readInt32(in, &uid);
        AParcel_readInt32(in, &serviceTypes);
        if (gState.processCallbacks.onForegroundServicesChanged) {
            gState.processCallbacks.onForegroundServicesChanged(pid, uid, serviceTypes);
        }
        return STATUS_OK;
    }
    if (code == gState.getCode(TxCode::OnProcessDied)) {
        int32_t pid = -1, uid = -1;
        AParcel_readInt32(in, &pid);
        AParcel_readInt32(in, &uid);
        if (gState.processCallbacks.onProcessDied) {
            gState.processCallbacks.onProcessDied(pid, uid);
        }
        return STATUS_OK;
    }
    uint32_t codeOnProcessStarted = gState.getCode(TxCode::OnProcessStarted);
    if (codeOnProcessStarted != 0 && code == codeOnProcessStarted) {
        int32_t pid = -1, processUid = -1, packageUid = -1;
        std::string packageName, processName;
        AParcel_readInt32(in, &pid);
        AParcel_readInt32(in, &processUid);
        AParcel_readInt32(in, &packageUid);
        AParcel_readString(in, &packageName, stringAllocator);
        AParcel_readString(in, &processName, stringAllocator);
        if (gState.processCallbacks.onProcessStarted) {
            gState.processCallbacks.onProcessStarted(pid, processUid, packageUid, packageName, processName);
        }
        return STATUS_OK;
    }
    return STATUS_UNKNOWN_ERROR;
}

static binder_status_t
displayCallback_transact(AIBinder *, uint32_t code, const AParcel *, AParcel *) {
    if (code != gState.getCode(TxCode::OnDisplayEvent)) return STATUS_UNKNOWN_ERROR;
    bool current = queryIsInteractive();
    if (current != gState.displayLastState) {
        gState.displayLastState = current;
        if (gState.displayCallback) gState.displayCallback(current);
    }
    return STATUS_OK;
}

static void pollPowerSave() {
    while (!gState.stopPowerSavePolling) {
        bool current = queryIsPowerSaveMode();
        if (current != gState.powerSaveLastState) {
            gState.powerSaveLastState = current;
            if (gState.powerSaveCallback) {
                gState.powerSaveCallback(current);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

// =============================================================================
// BinderMonitor
// =============================================================================

BinderMonitor &BinderMonitor::get() {
    static BinderMonitor instance;
    return instance;
}

BinderMonitor::~BinderMonitor() {
    gState.stopPowerSavePolling = true;
    if (gState.powerSavePollThread.joinable()) {
        gState.powerSavePollThread.join();
    }
}

bool BinderMonitor::initialize() {
    gState.isMiui = access("/data/data/com.miui.system", F_OK) == 0;
    LOGI_TAG("BinderMonitor", "MIUI device detected: {}", gState.isMiui);

    // Initialize fallbacks
    for (const auto &[code, q] : kResolverQueries) {
        gState.txCodes[code] = q.fallback;
    }

    LOGI_TAG("BinderMonitor", "Resolving transaction codes...");
    auto codes = runResolver();
    for (const auto &[code, q] : kResolverQueries) {
        auto it = codes.find(q.query);
        if (it != codes.end()) {
            gState.txCodes[code] = it->second;
        }
        if (q.isRequired && gState.txCodes[code] == 0 &&
            !(gState.isMiui && code == TxCode::IsPowerSaveMode)) {
            LOGE_TAG("BinderMonitor", "Failed to resolve required transaction code: {}", q.query);
            return false;
        }
    }

    if (gState.getCode(TxCode::OnProcessStarted) == 0) {
        LOGW_TAG("BinderMonitor", "TRANSACTION_onProcessStarted not resolved, onProcessStarted callback disabled");
    }
    LOGI_TAG("BinderMonitor", "All required transaction codes resolved");

    struct {
        const char *name;
        AIBinder **out;
    } services[] = {
            {"power",        &gState.powerBinder},
            {"notification", &gState.notificationBinder},
            {"activity",     &gState.activityBinder},
            {"display",      &gState.displayBinder},
            {"package",      &gState.packageBinder},
    };

    for (auto &s: services) {
        *s.out = AServiceManager_getService(s.name);
        if (!*s.out) {
            LOGW_TAG("BinderMonitor", "Service '{}' not immediately available, waiting...", s.name);
            *s.out = waitForServiceCompat(s.name);
        }
        if (!*s.out) {
            LOGE_TAG("BinderMonitor", "Failed to acquire service '{}'", s.name);
            return false;
        }
        LOGD_TAG("BinderMonitor", "Acquired service '{}'", s.name);
    }

    AIBinder_Class *powerClazz = AIBinder_Class_define(
            "android.os.IPowerManager", noopCreate, noopDestroy, noopTransact);
    AIBinder_associateClass(gState.powerBinder, powerClazz);

    AIBinder_Class *nmClazz = AIBinder_Class_define(
            "android.app.INotificationManager", noopCreate, noopDestroy, noopTransact);
    AIBinder_associateClass(gState.notificationBinder, nmClazz);

    AIBinder_Class *amClazz = AIBinder_Class_define(
            "android.app.IActivityManager", noopCreate, noopDestroy, noopTransact);
    AIBinder_associateClass(gState.activityBinder, amClazz);

    AIBinder_Class *dmClazz = AIBinder_Class_define(
            "android.hardware.display.IDisplayManager", noopCreate, noopDestroy, noopTransact);
    AIBinder_associateClass(gState.displayBinder, dmClazz);

    AIBinder_Class *pmClazz = AIBinder_Class_define(
            "android.content.pm.IPackageManager", noopCreate, noopDestroy, noopTransact);
    AIBinder_associateClass(gState.packageBinder, pmClazz);

    AIBinder_Class *processObserverClazz = AIBinder_Class_define(
            "android.app.IProcessObserver", noopCreate, noopDestroy, processObserver_transact);
    gState.processObserverBinder = AIBinder_new(processObserverClazz, nullptr);

    if (!gState.processObserverBinder) {
        LOGE_TAG("BinderMonitor", "Failed to allocate IProcessObserver binder");
        return false;
    }

    {
        AParcel *in = nullptr, *out = nullptr;
        if (AIBinder_prepareTransaction(gState.activityBinder, &in) == STATUS_OK) {
            AParcel_writeInterfaceToken(in, "android.app.IActivityManager");
            AParcel_writeStrongBinder(in, gState.processObserverBinder);
            binder_status_t status = AIBinder_transact(
                    gState.activityBinder, gState.getCode(TxCode::RegisterProcessObserver), &in, &out, 0);
            if (status != STATUS_OK) {
                LOGE_TAG("BinderMonitor", "registerProcessObserver transaction failed, status={}", status);
            } else {
                LOGI_TAG("BinderMonitor", "Process observer registered successfully");
            }
            if (out) AParcel_delete(out);
        } else {
            LOGE_TAG("BinderMonitor", "prepareTransaction failed for registerProcessObserver");
        }
    }

    AIBinder_Class *displayCallbackClazz = AIBinder_Class_define(
            "android.hardware.display.IDisplayManagerCallback",
            noopCreate, noopDestroy, displayCallback_transact);
    gState.displayCallbackBinder = AIBinder_new(displayCallbackClazz, nullptr);
    if (!gState.displayCallbackBinder) {
        LOGE_TAG("BinderMonitor", "Failed to allocate IDisplayManagerCallback binder");
        return false;
    }

    gState.displayLastState = queryIsInteractive();
    if (gState.displayCallback) gState.displayCallback(gState.displayLastState);

    {
        AParcel *in = nullptr, *out = nullptr;
        if (AIBinder_prepareTransaction(gState.displayBinder, &in) == STATUS_OK) {
            AParcel_writeInterfaceToken(in, "android.hardware.display.IDisplayManager");
            AParcel_writeStrongBinder(in, gState.displayCallbackBinder);
            binder_status_t status = AIBinder_transact(
                    gState.displayBinder,
                    gState.getCode(TxCode::RegisterDisplayCallback),
                    &in,
                    &out,
                    0
            );
            if (status != STATUS_OK) {
                LOGE_TAG("BinderMonitor", "registerCallback (display) transaction failed, status={}", status);
            } else {
                LOGI_TAG("BinderMonitor", "Display callback registered successfully");
            }
            if (out) AParcel_delete(out);
        } else {
            LOGE_TAG("BinderMonitor", "prepareTransaction failed for registerCallback (display)");
        }
    }

    // Initialize Power Save state and start polling thread
    gState.powerSaveLastState = queryIsPowerSaveMode();
    if (gState.powerSaveCallback) {
        gState.powerSaveCallback(gState.powerSaveLastState);
    }

    gState.stopPowerSavePolling = false;
    gState.powerSavePollThread = std::thread(pollPowerSave);

    ABinderProcess_startThreadPool();
    LOGI_TAG("BinderMonitor", "BinderMonitor initialized");
    return true;
}

void BinderMonitor::setProcessObserverCallbacks(ProcessObserverCallbacks callbacks) {
    gState.processCallbacks = std::move(callbacks);
}

void BinderMonitor::setDisplayStateCallback(DisplayStateCallback callback) {
    gState.displayCallback = std::move(callback);
}

void BinderMonitor::setPowerSaveCallback(PowerSaveCallback callback) {
    gState.powerSaveCallback = std::move(callback);
}

bool BinderMonitor::isPowerSave() {
    if (!gState.powerBinder) {
        LOGE_TAG("BinderMonitor", "isPowerSave called before successful initialize()");
        return false;
    }
    uint32_t tx = gState.getCode(TxCode::IsPowerSaveMode);
    if (!tx) return false;
    return transactReadInt32(gState.powerBinder, tx, "android.os.IPowerManager", 0) != 0;
}

int32_t BinderMonitor::getZenMode() {
    if (!gState.notificationBinder) {
        LOGE_TAG("BinderMonitor", "getZenMode called before successful initialize()");
        return -1;
    }
    uint32_t tx = gState.getCode(TxCode::GetZenMode);
    if (!tx) return -1;
    return transactReadInt32(gState.notificationBinder, tx, "android.app.INotificationManager");
}

std::string BinderMonitor::getPackageNameForUid(int32_t uid) {
    if (!gState.packageBinder) {
        LOGE_TAG("BinderMonitor", "getPackageNameForUid called before successful initialize()");
        return "";
    }
    uint32_t tx = gState.getCode(TxCode::GetPackageNameForUid);
    if (!tx) return "";
    return transactReadString(gState.packageBinder, tx, "android.content.pm.IPackageManager", uid);
}

void BinderMonitor::joinThreadPool() {
    ABinderProcess_joinThreadPool();
}
