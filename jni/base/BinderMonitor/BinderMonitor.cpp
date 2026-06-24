#include "BinderMonitor.hpp"
#include "BinderNDK.hpp"
#include "Encore.hpp"
#include "EncoreLog.hpp"

#include <cstring>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unistd.h>
#include <sys/wait.h>

// =============================================================================
// Internal state
// =============================================================================

struct BinderMonitorState {
    AIBinder *powerBinder = nullptr;
    AIBinder *notificationBinder = nullptr;
    AIBinder *activityBinder = nullptr;
    AIBinder *displayBinder = nullptr;

    // Held alive so the remote services keep strong refs to our callbacks.
    AIBinder *processObserverBinder = nullptr;
    AIBinder *displayCallbackBinder = nullptr;

    uint32_t txIsPowerSaveMode = 0;
    uint32_t txIsInteractive = 0;
    uint32_t txGetZenMode = 0;
    uint32_t txRegisterProcessObserver = 0;
    uint32_t txRegisterDisplayCallback = 0;

    uint32_t codeOnForegroundActivitiesChanged = 0;
    uint32_t codeOnForegroundServicesChanged = 0;
    uint32_t codeOnProcessDied = 0;
    uint32_t codeOnProcessStarted = 0; // 0 = not available (pre-API 33)
    uint32_t codeOnDisplayEvent = 1; // fallback when resolver cannot find it

    ProcessObserverCallbacks processCallbacks;
    DisplayStateCallback displayCallback;
    bool displayLastState = false;
};

static BinderMonitorState gState;

// =============================================================================
// Runtime transaction code resolver
// =============================================================================

struct ResolverTarget {
    const char *query;
    uint32_t *target;
    bool isRequired;
    uint32_t fallback = 0;
};

static const ResolverTarget kResolverTargets[] = {
        {"android.os.IPowerManager.Stub::TRANSACTION_isPowerSaveMode", &gState.txIsPowerSaveMode, true},
        {"android.os.IPowerManager.Stub::TRANSACTION_isInteractive", &gState.txIsInteractive, true},
        {"android.app.INotificationManager.Stub::TRANSACTION_getZenMode", &gState.txGetZenMode, true},
        {"android.app.IActivityManager.Stub::TRANSACTION_registerProcessObserver", &gState.txRegisterProcessObserver, true},
        {"android.hardware.display.IDisplayManager.Stub::TRANSACTION_registerCallback", &gState.txRegisterDisplayCallback, true},
        {"android.app.IProcessObserver.Stub::TRANSACTION_onForegroundActivitiesChanged", &gState.codeOnForegroundActivitiesChanged, true},
        {"android.app.IProcessObserver.Stub::TRANSACTION_onForegroundServicesChanged", &gState.codeOnForegroundServicesChanged, true},
        {"android.app.IProcessObserver.Stub::TRANSACTION_onProcessDied", &gState.codeOnProcessDied, true},
        {"android.app.IProcessObserver.Stub::TRANSACTION_onProcessStarted", &gState.codeOnProcessStarted, false, 0},
        {"android.hardware.display.IDisplayManagerCallback.Stub::TRANSACTION_onDisplayEvent", &gState.codeOnDisplayEvent, false, 1},
};

static std::unordered_map <std::string, uint32_t> runResolver(const char *apkPath) {
    std::unordered_map <std::string, uint32_t> result;

    int stdinPipe[2], stdoutPipe[2];
    if (pipe(stdinPipe) != 0 || pipe(stdoutPipe) != 0) {
        LOGE_TAG("BinderMonitor", "Failed to create pipes for resolver subprocess");
        return result;
    }

    pid_t child = fork();
    if (child < 0) {
        LOGE_TAG("BinderMonitor", "fork() failed for resolver subprocess");
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        return result;
    }

    if (child == 0) {
        dup2(stdinPipe[0], STDIN_FILENO);
        dup2(stdoutPipe[1], STDOUT_FILENO);
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);

        std::string cp = std::string("-Djava.class.path=") + apkPath;
        execl("/system/bin/app_process", "app_process",
              cp.c_str(), "/",
              "com.rem01gaming.binderresolver.MainKt",
              nullptr);
        _exit(1);
    }

    close(stdinPipe[0]);
    close(stdoutPipe[1]);

    for (const auto &target : kResolverTargets) {
        write(stdinPipe[1], target.query, strlen(target.query));
        write(stdinPipe[1], "\n", 1);
    }
    close(stdinPipe[1]);

    std::string output;
    char buf[512];
    ssize_t n;
    while ((n = read(stdoutPipe[0], buf, sizeof(buf))) > 0) {
        output.append(buf, static_cast<size_t>(n));
    }
    close(stdoutPipe[0]);

    int status;
    waitpid(child, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        LOGE_TAG("BinderMonitor", "Resolver subprocess exited with code {}", WEXITSTATUS(status));
    }

    std::istringstream ss(output);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.rfind("ERROR:", 0) == 0) {
            LOGW_TAG("BinderMonitor", "Resolver: {}", line);
            continue;
        }

        auto sp = line.rfind(' ');
        if (sp == std::string::npos) continue;

        try {
            result[line.substr(0, sp)] = static_cast<uint32_t>(std::stoul(line.substr(sp + 1)));
        } catch (...) {
            LOGW_TAG("BinderMonitor", "Failed to parse resolver output line: {}", line);
        }
    }

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
 *
 * @param binder   Target remote binder.
 * @param tx       Transaction code.
 * @param ifToken  Interface descriptor token written into the request parcel.
 * @param onError  Value returned on any failure path.
 * @return The int32 value from the reply parcel, or onError.
 */
static int32_t
transactReadInt32(AIBinder *binder, uint32_t tx, const char *ifToken, int32_t onError = -1) {
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

static bool queryIsInteractive() {
    return transactReadInt32(gState.powerBinder, gState.txIsInteractive, "android.os.IPowerManager",
                             0) != 0;
}

// =============================================================================
// Binder onTransact callbacks
// =============================================================================

static binder_status_t processObserverTransact(AIBinder *, uint32_t code,
                                               const AParcel *in, AParcel *) {
    if (code == gState.codeOnForegroundActivitiesChanged) {
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

    if (code == gState.codeOnForegroundServicesChanged) {
        int32_t pid = -1, uid = -1, serviceTypes = 0;
        AParcel_readInt32(in, &pid);
        AParcel_readInt32(in, &uid);
        AParcel_readInt32(in, &serviceTypes);

        if (gState.processCallbacks.onForegroundServicesChanged) {
            gState.processCallbacks.onForegroundServicesChanged(pid, uid, serviceTypes);
        }

        return STATUS_OK;
    }

    if (code == gState.codeOnProcessDied) {
        int32_t pid = -1, uid = -1;
        AParcel_readInt32(in, &pid);
        AParcel_readInt32(in, &uid);

        if (gState.processCallbacks.onProcessDied) {
            gState.processCallbacks.onProcessDied(pid, uid);
        }

        return STATUS_OK;
    }

    // codeOnProcessStarted == 0 means the resolver did not find it (pre-API 33).
    if (gState.codeOnProcessStarted != 0 && code == gState.codeOnProcessStarted) {
        int32_t pid = -1, processUid = -1, packageUid = -1;
        std::string packageName, processName;
        AParcel_readInt32(in, &pid);
        AParcel_readInt32(in, &processUid);
        AParcel_readInt32(in, &packageUid);
        AParcel_readString(in, &packageName, stringAllocator);
        AParcel_readString(in, &processName, stringAllocator);

        if (gState.processCallbacks.onProcessStarted) {
            gState.processCallbacks.onProcessStarted(pid, processUid, packageUid, packageName,
                                                     processName);
        }

        return STATUS_OK;
    }

    return STATUS_UNKNOWN_ERROR;
}

static binder_status_t
displayCallbackTransact(AIBinder *, uint32_t code, const AParcel *, AParcel *) {
    if (code != gState.codeOnDisplayEvent) return STATUS_UNKNOWN_ERROR;

    bool current = queryIsInteractive();
    if (current != gState.displayLastState) {
        gState.displayLastState = current;
        if (gState.displayCallback) gState.displayCallback(current);
    }

    return STATUS_OK;
}

// =============================================================================
// BinderMonitor
// =============================================================================

BinderMonitor &BinderMonitor::get() {
    static BinderMonitor instance;
    return instance;
}

bool BinderMonitor::initialize() {
    auto codes = runResolver(MODPATH "/binder_monitor.apk");

    for (const auto &t : kResolverTargets) {
        auto it = codes.find(t.query);
        *t.target = (it != codes.end()) ? it->second : t.fallback;

        if (t.isRequired && *t.target == 0) {
            LOGE_TAG("BinderMonitor", "Failed to resolve required transaction code: {}", t.query);
            return false;
        }
    }

    if (!gState.codeOnProcessStarted) {
        LOGW_TAG("BinderMonitor",
                 "TRANSACTION_onProcessStarted not resolved, onProcessStarted callback disabled");
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
    };

    for (auto &s: services) {
        *s.out = AServiceManager_getService(s.name);
        if (!*s.out) {
            LOGW_TAG("BinderMonitor", "Service '{}' not immediately available, waiting...", s.name);
            *s.out = AServiceManager_waitForService(s.name);
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

    AIBinder_Class *processObserverClazz = AIBinder_Class_define(
            "android.app.IProcessObserver", noopCreate, noopDestroy, processObserverTransact);

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
                    gState.activityBinder, gState.txRegisterProcessObserver, &in, &out, 0);
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
            noopCreate, noopDestroy, displayCallbackTransact);

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
                    gState.txRegisterDisplayCallback,
                    &in,
                    &out,
                    0
            );

            if (status != STATUS_OK) {
                LOGE_TAG("BinderMonitor", "registerCallback (display) transaction failed, status={}",
                         status);
            } else {
                LOGI_TAG("BinderMonitor", "Display callback registered successfully");
            }

            if (out) AParcel_delete(out);
        } else {
            LOGE_TAG("BinderMonitor", "prepareTransaction failed for registerCallback (display)");
        }
    }

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

bool BinderMonitor::isPowerSave() {
    if (!gState.powerBinder || !gState.txIsPowerSaveMode) {
        LOGE_TAG("BinderMonitor", "isPowerSave called before successful initialize()");
        return false;
    }

    return transactReadInt32(gState.powerBinder, gState.txIsPowerSaveMode,
                             "android.os.IPowerManager", 0) != 0;
}

int32_t BinderMonitor::getZenMode() {
    if (!gState.notificationBinder || !gState.txGetZenMode) {
        LOGE_TAG("BinderMonitor", "getZenMode called before successful initialize()");
        return -1;
    }

    return transactReadInt32(gState.notificationBinder, gState.txGetZenMode,
                             "android.app.INotificationManager");
}

void BinderMonitor::joinThreadPool() {
    ABinderProcess_joinThreadPool();
}