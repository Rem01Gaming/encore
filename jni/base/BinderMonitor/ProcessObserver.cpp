#include "BinderNDK.hpp"
#include <iostream>
#include <string>
#include <unistd.h>

// Transaction codes should be resolved at runtime, value changes each Android version

static uint32_t TX_REGISTER_PROCESS_OBSERVER = 110;

static uint32_t CODE_ON_FOREGROUND_ACTIVITIES_CHANGED = 1;
static uint32_t CODE_ON_FOREGROUND_SERVICES_CHANGED = 2;
static uint32_t CODE_ON_PROCESS_DIED = 3;
static uint32_t CODE_ON_PROCESS_STARTED = 0; // API 33+ only

/**
 * @brief Allocator callback for receiving string data from an AParcel.
 * @param stringData Pointer to the target std::string instance.
 * @param size The size of the string including the null terminator.
 * @param out Output pointer pointing to the internal string buffer.
 * @return true if allocation succeeded, false otherwise.
 */
static bool string_allocator(void *stringData, int32_t size, char **out) {
    if (size < 0) return false;
    auto *str = static_cast<std::string *>(stringData);
    str->resize(size);
    *out = &(*str)[0];
    return true;
}

/**
 * @brief Handles incoming binder transactions for the custom IProcessObserver object.
 * @param binder The binder instance receiving the transaction.
 * @param code The interface transaction ID.
 * @param in The incoming data parcel.
 * @param out The outgoing response parcel.
 * @return STATUS_OK if the transaction was handled successfully.
 */
static binder_status_t observerOnTransact(AIBinder *binder, uint32_t code, const AParcel *in, AParcel *out) {
    (void)binder;
    (void)out;

    if (code == CODE_ON_FOREGROUND_ACTIVITIES_CHANGED) {
        int32_t pid = -1;
        int32_t uid = -1;
        bool foreground = false;

        AParcel_readInt32(in, &pid);
        AParcel_readInt32(in, &uid);
        AParcel_readBool(in, &foreground);

        std::cout << "[onForegroundActivitiesChanged] PID: " << pid
                  << " | UID: " << uid
                  << " | Foreground: " << (foreground ? "TRUE" : "FALSE")
                  << std::endl;

        return STATUS_OK;
    }

    if (code == CODE_ON_FOREGROUND_SERVICES_CHANGED) {
        int32_t pid = -1;
        int32_t uid = -1;
        int32_t serviceTypes = 0;

        AParcel_readInt32(in, &pid);
        AParcel_readInt32(in, &uid);
        AParcel_readInt32(in, &serviceTypes);

        std::cout << "[onForegroundServicesChanged] PID: " << pid
                  << " | UID: " << uid
                  << " | ServiceTypes Bitmask: " << serviceTypes
                  << std::endl;

        return STATUS_OK;
    }

    if (code == CODE_ON_PROCESS_DIED) {
        int32_t pid = -1;
        int32_t uid = -1;

        AParcel_readInt32(in, &pid);
        AParcel_readInt32(in, &uid);

        std::cout << "[onProcessDied] PID: " << pid
                  << " | UID: " << uid
                  << std::endl;

        return STATUS_OK;
    }

    if (CODE_ON_PROCESS_STARTED != 0 && code == CODE_ON_PROCESS_STARTED) {
        int32_t pid = -1;
        int32_t processUid = -1;
        int32_t packageUid = -1;
        std::string packageName;
        std::string processName;

        AParcel_readInt32(in, &pid);
        AParcel_readInt32(in, &processUid);
        AParcel_readInt32(in, &packageUid);
        AParcel_readString(in, &packageName, string_allocator);
        AParcel_readString(in, &processName, string_allocator);

        std::cout << "[onProcessStarted] PID: " << pid
                  << " | ProcUID: " << processUid
                  << " | PkgUID: " << packageUid
                  << " | Package: " << packageName
                  << " | Process: " << processName
                  << std::endl;

        return STATUS_OK;
    }

    return STATUS_UNKNOWN_ERROR;
}

/**
 * @brief Dummy setup initialization for the binder class hook.
 */
static void *observerOnCreate(void *args) {
    return args;
}

/**
 * @brief Dummy teardown cleanup for the binder class hook.
 */
static void observerOnDestroy(void *userData) {
    (void)userData;
}

/**
 * @brief Registers the custom process observer proxy object into ActivityManagerService.
 * @param amBinder The remote ActivityManager IBinder reference.
 * @param observer The local custom IProcessObserver instantiation.
 * @return true if registration transaction executed without exception.
 */
static bool registerProcessObserver(AIBinder *amBinder, AIBinder *observer) {
    AParcel *inParcel = nullptr;
    AParcel *outParcel = nullptr;

    if (AIBinder_prepareTransaction(amBinder, &inParcel) != STATUS_OK) return false;

    AParcel_writeInterfaceToken(inParcel, "android.app.IActivityManager");
    AParcel_writeStrongBinder(inParcel, observer);

    binder_status_t status = AIBinder_transact(
            amBinder,
            TX_REGISTER_PROCESS_OBSERVER,
            &inParcel,
            &outParcel,
            0);

    if (status != STATUS_OK) {
        if (outParcel) AParcel_delete(outParcel);
        return false;
    }

    AParcel_delete(outParcel);
    return true;
}

int main() {
    AIBinder *activityManager = AServiceManager_getService("activity");
    if (!activityManager) {
        activityManager = AServiceManager_waitForService("activity");
    }

    if (!activityManager) {
        std::cerr << "Failed to find ActivityManager service" << std::endl;
        return 1;
    }

    AIBinder_Class *amClazz = AIBinder_Class_define(
            "android.app.IActivityManager",
            observerOnCreate,
            observerOnDestroy,
            observerOnTransact);
    AIBinder_associateClass(activityManager, amClazz);

    AIBinder_Class *observerClazz = AIBinder_Class_define(
            "android.app.IProcessObserver",
            observerOnCreate,
            observerOnDestroy,
            observerOnTransact);

    AIBinder *myObserver = AIBinder_new(observerClazz, nullptr);
    if (!myObserver) {
        std::cerr << "Failed to allocate local observer interface" << std::endl;
        return 1;
    }

    ABinderProcess_startThreadPool();

    if (!registerProcessObserver(activityManager, myObserver)) {
        std::cerr << "Registration failed. Verify SET_ACTIVITY_WATCHER permissions." << std::endl;
        return 1;
    }

    std::cout << "Successfully registered process observer. Listening for events..." << std::endl;

    while (true) {
        pause();
    }

    return 0;
}
