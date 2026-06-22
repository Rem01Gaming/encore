#include "BinderNDK.hpp"

#include <iostream>

static uint32_t TX_GET_ZEN_MODE = 113;

static void *noopOnCreate(void *args) {
    return args;
}
static void noopOnDestroy(void *) {
}
static binder_status_t noopOnTransact(AIBinder *, uint32_t, const AParcel *, AParcel *) {
    return STATUS_UNKNOWN_ERROR;
}

/**
 * @brief Queries the current zen mode state from NotificationManagerService.
 *
 * @param nmBinder Remote INotificationManager binder reference.
 * @return Zen mode integer (0 = off, non-zero = active), or -1 on failure.
 */
static int32_t getZenMode(AIBinder *nmBinder) {
    AParcel *inParcel = nullptr;
    AParcel *outParcel = nullptr;

    if (AIBinder_prepareTransaction(nmBinder, &inParcel) != STATUS_OK) return -1;

    AParcel_writeInterfaceToken(inParcel, "android.app.INotificationManager");

    binder_status_t status = AIBinder_transact(nmBinder, TX_GET_ZEN_MODE, &inParcel, &outParcel, 0);
    if (status != STATUS_OK) {
        if (outParcel) AParcel_delete(outParcel);
        return -1;
    }

    AStatus *replyStatus = nullptr;
    AParcel_readStatusHeader(outParcel, &replyStatus);
    bool ok = replyStatus && AStatus_isOk(replyStatus);
    if (replyStatus) AStatus_delete(replyStatus);

    if (!ok) {
        AParcel_delete(outParcel);
        return -1;
    }

    int32_t zenMode = -1;
    AParcel_readInt32(outParcel, &zenMode);
    AParcel_delete(outParcel);
    return zenMode;
}

int main() {
    AIBinder *notificationManager = AServiceManager_getService("notification");
    if (!notificationManager) {
        notificationManager = AServiceManager_waitForService("notification");
    }
    if (!notificationManager) {
        std::cerr << "Failed to find notification service" << std::endl;
        return 1;
    }

    AIBinder_Class *nmClazz = AIBinder_Class_define(
            "android.app.INotificationManager",
            noopOnCreate,
            noopOnDestroy,
            noopOnTransact);
    AIBinder_associateClass(notificationManager, nmClazz);

    int32_t zenMode = getZenMode(notificationManager);
    if (zenMode < 0) {
        std::cerr << "getZenMode failed, check TX code or ACCESS_NOTIFICATION_POLICY permission." << std::endl;
        AIBinder_decStrong(notificationManager);
        return 1;
    }

    std::cout << zenMode << std::endl;

    AIBinder_decStrong(notificationManager);
    return 0;
}
