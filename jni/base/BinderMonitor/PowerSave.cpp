#include "BinderNDK.hpp"
#include <iostream>

static uint32_t TX_IS_POWER_SAVE_MODE = 16;

/**
 * @brief Queries the current Battery Saver (Power Save Mode) state from PowerManagerService.
 *
 * @param powerBinder Remote reference to the PowerManagerService binder.
 * @return True if Battery Saver is active, false otherwise.
 */
static bool checkIsPowerSaveMode(AIBinder *powerBinder) {
    if (!powerBinder) return false;

    AParcel *inParcel = nullptr;
    AParcel *outParcel = nullptr;

    if (AIBinder_prepareTransaction(powerBinder, &inParcel) != STATUS_OK) return false;

    AParcel_writeInterfaceToken(inParcel, "android.os.IPowerManager");

    binder_status_t status = AIBinder_transact(powerBinder, TX_IS_POWER_SAVE_MODE, &inParcel, &outParcel, 0);
    if (status != STATUS_OK) {
        if (outParcel) AParcel_delete(outParcel);
        return false;
    }

    AStatus *replyStatus = nullptr;
    AParcel_readStatusHeader(outParcel, &replyStatus);
    bool ok = replyStatus && AStatus_isOk(replyStatus);
    if (replyStatus) AStatus_delete(replyStatus);

    if (!ok) {
        AParcel_delete(outParcel);
        return false;
    }

    int32_t result = 0;
    AParcel_readInt32(outParcel, &result);
    AParcel_delete(outParcel);

    return result != 0;
}

/**
 * @brief Context creation callback for the custom Binder class.
 *
 * @param args User arguments passed during instantiation.
 * @return Pointer to the allocated context or arguments.
 */
static void *noopOnCreate(void *args) {
    return args;
}

/**
 * @brief Context destruction callback for the custom Binder class.
 *
 * @param context Pointer to the context being destroyed.
 */
static void noopOnDestroy(void *) {
}

/**
 * @brief Fallback transaction handler for the proxy interface block.
 * * @return Always returns STATUS_UNKNOWN_ERROR.
 */
static binder_status_t noopOnTransact(AIBinder*, uint32_t, const AParcel*, AParcel*) {
    return STATUS_UNKNOWN_ERROR;
}

int main() {
    AIBinder *powerBinder = AServiceManager_getService("power");
    if (!powerBinder) {
        powerBinder = AServiceManager_waitForService("power");
    }

    if (!powerBinder) {
        std::cerr << "Failed to acquire power system service" << std::endl;
        return 1;
    }

    AIBinder_Class *powerManagerClazz = AIBinder_Class_define(
            "android.os.IPowerManager",
            noopOnCreate,
            noopOnDestroy,
            noopOnTransact);
    AIBinder_associateClass(powerBinder, powerManagerClazz);

    bool isSaveModeActive = checkIsPowerSaveMode(powerBinder);
    std::cout << (isSaveModeActive ? "true" : "false") << std::endl;

    AIBinder_decStrong(powerBinder);
    return 0;
}
