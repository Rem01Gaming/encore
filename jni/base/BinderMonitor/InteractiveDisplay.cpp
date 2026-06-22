#include "BinderNDK.hpp"
#include <iostream>

static uint32_t TX_REGISTER_CALLBACK = 4;
static uint32_t TX_IS_INTERACTIVE = 15;

static AIBinder *gPowerBinder = nullptr;
static bool gLastState = false;
static bool gFirstCheck = true;

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
 * @brief Default no-op transaction handler for dummy binder interfaces.
 *
 * @return STATUS_UNKNOWN_ERROR always.
 */
static binder_status_t noopOnTransact(AIBinder *, uint32_t, const AParcel *, AParcel *) {
    return STATUS_UNKNOWN_ERROR;
}

/**
 * @brief Queries the current interactivity state from PowerManagerService.
 *
 * @return True if the screen is interactive, false otherwise.
 */
static bool checkIsInteractive() {
    if (!gPowerBinder) return false;

    AParcel *inParcel = nullptr;
    AParcel *outParcel = nullptr;

    if (AIBinder_prepareTransaction(gPowerBinder, &inParcel) != STATUS_OK) return false;

    AParcel_writeInterfaceToken(inParcel, "android.os.IPowerManager");

    binder_status_t status = AIBinder_transact(gPowerBinder, TX_IS_INTERACTIVE, &inParcel, &outParcel, 0);
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
 * @brief Handles incoming binder transactions for the display manager callback.
 *
 * @param binder Remote reference to the transaction target binder.
 * @param code The transaction code identifier.
 * @param in Input parameter data parcel.
 * @param out Output reply data parcel.
 * @return STATUS_OK on success, or an error status code on failure.
 */
static binder_status_t callbackOnTransact(AIBinder *, uint32_t code, const AParcel *, AParcel *) {
    if (code == 1) { // IDisplayManagerCallback.Stub.TRANSACTION_onDisplayEvent
        bool currentState = checkIsInteractive();
        if (gFirstCheck || currentState != gLastState) {
            std::cout << (currentState ? "true" : "false") << std::endl;
            gLastState = currentState;
            gFirstCheck = false;
        }
        return STATUS_OK;
    }
    return STATUS_UNKNOWN_ERROR;
}

int main() {
    gPowerBinder = AServiceManager_getService("power");
    if (!gPowerBinder) {
        gPowerBinder = AServiceManager_waitForService("power");
    }

    AIBinder *displayBinder = AServiceManager_getService("display");
    if (!displayBinder) {
        displayBinder = AServiceManager_waitForService("display");
    }

    if (!gPowerBinder || !displayBinder) {
        std::cerr << "Failed to acquire system services" << std::endl;
        return 1;
    }

    AIBinder_Class *displayManagerClazz = AIBinder_Class_define(
            "android.hardware.display.IDisplayManager",
            noopOnCreate,
            noopOnDestroy,
            noopOnTransact);
    AIBinder_associateClass(displayBinder, displayManagerClazz);

    AIBinder_Class *powerManagerClazz = AIBinder_Class_define(
            "android.os.IPowerManager",
            noopOnCreate,
            noopOnDestroy,
            noopOnTransact);
    AIBinder_associateClass(gPowerBinder, powerManagerClazz);

    AIBinder_Class *callbackClazz = AIBinder_Class_define(
            "android.hardware.display.IDisplayManagerCallback",
            noopOnCreate,
            noopOnDestroy,
            callbackOnTransact);

    AIBinder *callbackBinder = AIBinder_new(callbackClazz, nullptr);

    gLastState = checkIsInteractive();
    std::cout << (gLastState ? "true" : "false") << std::endl;
    gFirstCheck = false;

    AParcel *inParcel = nullptr;
    AParcel *outParcel = nullptr;

    if (AIBinder_prepareTransaction(displayBinder, &inParcel) == STATUS_OK) {
        AParcel_writeInterfaceToken(inParcel, "android.hardware.display.IDisplayManager");
        AParcel_writeStrongBinder(inParcel, callbackBinder);

        AIBinder_transact(displayBinder, TX_REGISTER_CALLBACK, &inParcel, &outParcel, 0);
        if (outParcel) AParcel_delete(outParcel);
    }

    ABinderProcess_startThreadPool();
    ABinderProcess_joinThreadPool();

    return 0;
}
