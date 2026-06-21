#pragma once

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

struct AIBinder;
struct AIBinder_Class;
struct AIBinder_DeathRecipient;
struct AIBinder_Weak;
struct AParcel;
struct AStatus;

using binder_status_t = int32_t;
using binder_flags_t = uint32_t;
using transaction_code_t = uint32_t;
using binder_exception_t = int32_t;

using AIBinder_Class_onCreate = void *(*)(void *);
using AIBinder_Class_onDestroy = void (*)(void *);
using AIBinder_Class_onTransact = binder_status_t (*)(AIBinder *, uint32_t, const AParcel *, AParcel *);
using AIBinder_DeathRecipient_onBinderDied = void (*)(void *);

using AParcel_charArrayAllocator = bool (*)(void *, int32_t, char16_t **);
using AParcel_stringArrayAllocator = bool (*)(void *, int32_t, int32_t, char **);

// =============================================================================
// Binder Flags & Transaction Constants
// =============================================================================

enum {
    FLAG_ONEWAY = 0x01
};

enum {
    FIRST_CALL_TRANSACTION = 0x00000001,
    LAST_CALL_TRANSACTION = 0x00ffffff
};

// =============================================================================
// Low-level status types for use in binder
// =============================================================================

enum {
    STATUS_OK = 0,
    STATUS_UNKNOWN_ERROR = (-2147483647 - 1),
    STATUS_NO_MEMORY = -ENOMEM,
    STATUS_INVALID_OPERATION = -ENOSYS,
    STATUS_BAD_VALUE = -EINVAL,
    STATUS_BAD_TYPE = (STATUS_UNKNOWN_ERROR + 1),
    STATUS_NAME_NOT_FOUND = -ENOENT,
    STATUS_PERMISSION_DENIED = -EPERM,
    STATUS_NO_INIT = -ENODEV,
    STATUS_ALREADY_EXISTS = -EEXIST,
    STATUS_DEAD_OBJECT = -EPIPE,
    STATUS_FAILED_TRANSACTION = (STATUS_UNKNOWN_ERROR + 2),
    STATUS_BAD_INDEX = -EOVERFLOW,
    STATUS_NOT_ENOUGH_DATA = -ENOMEM,
    STATUS_WOULD_BLOCK = -EWOULDBLOCK,
    STATUS_TIMED_OUT = -ETIMEDOUT,
    STATUS_UNKNOWN_TRANSACTION = -EBADMSG,
    STATUS_FDS_NOT_ALLOWED = (STATUS_UNKNOWN_ERROR + 7),
    STATUS_UNEXPECTED_NULL = (STATUS_UNKNOWN_ERROR + 8)
};

// =============================================================================
// Top level exceptions types for Android binder errors (maps to Java)
// =============================================================================

enum {
    EX_NONE = 0,
    EX_SECURITY = -1,
    EX_BAD_PARCELABLE = -2,
    EX_ILLEGAL_ARGUMENT = -3,
    EX_NULL_POINTER = -4,
    EX_ILLEGAL_STATE = -5,
    EX_NETWORK_MAIN_THREAD = -6,
    EX_UNSUPPORTED_OPERATION = -7,
    EX_SERVICE_SPECIFIC = -8,
    EX_PARCELABLE = -9,
    EX_TRANSACTION_FAILED = -129
};

extern "C" {

// =============================================================================
// Service Manager
// =============================================================================

AIBinder *AServiceManager_getService(const char *instance);
AIBinder *AServiceManager_waitForService(const char *instance);
binder_status_t AServiceManager_addService(AIBinder *binder, const char *instance);
AIBinder *AServiceManager_checkService(const char *instance);
bool AServiceManager_isDeclared(const char *instance);
void AServiceManager_forEachDeclaredInstance(const char *descriptor, void *context, void (*callback)(const char *, void *));

// =============================================================================
// IBinder Class Definition
// =============================================================================

AIBinder_Class *AIBinder_Class_define(const char *interfaceDescriptor, AIBinder_Class_onCreate onCreate, AIBinder_Class_onDestroy onDestroy, AIBinder_Class_onTransact onTransact);
const char *AIBinder_Class_getDescriptor(const AIBinder_Class *clazz);
void AIBinder_Class_disableInterfaceTokenHeader(AIBinder_Class *clazz);

// =============================================================================
// IBinder Lifecycle
// =============================================================================

AIBinder *AIBinder_new(const AIBinder_Class *clazz, void *args);
bool AIBinder_associateClass(AIBinder *binder, const AIBinder_Class *clazz);
void AIBinder_incStrong(AIBinder *binder);
void AIBinder_decStrong(AIBinder *binder);
const AIBinder_Class *AIBinder_getClass(AIBinder *binder);
void *AIBinder_getUserData(AIBinder *binder);

// =============================================================================
// IBinder State Checks
// =============================================================================

bool AIBinder_isAlive(const AIBinder *binder);
bool AIBinder_isRemote(const AIBinder *binder);
bool AIBinder_isHandlingTransaction();

// =============================================================================
// Caller Identity
// =============================================================================

uid_t AIBinder_getCallingUid();
pid_t AIBinder_getCallingPid();
const char *AIBinder_getCallingSid();

// =============================================================================
// Thread Pool
// =============================================================================

void ABinderProcess_startThreadPool();
void ABinderProcess_joinThreadPool();
void ABinderProcess_setThreadPoolMaxThreadCount(uint32_t numThreads);
bool ABinderProcess_isThreadPoolStarted();

// =============================================================================
// Transaction
// =============================================================================

binder_status_t AIBinder_prepareTransaction(AIBinder *binder, AParcel **in);
binder_status_t AIBinder_transact(AIBinder *binder, transaction_code_t code, AParcel **in, AParcel **out, binder_flags_t flags);

// =============================================================================
// Death Notifications
// =============================================================================

AIBinder_DeathRecipient *AIBinder_DeathRecipient_new(AIBinder_DeathRecipient_onBinderDied onBinderDied);
void AIBinder_DeathRecipient_delete(AIBinder_DeathRecipient *recipient);
void AIBinder_DeathRecipient_setOnUnlinked(AIBinder_DeathRecipient *recipient, void (*onUnlinked)(void *));
binder_status_t AIBinder_linkToDeath(AIBinder *binder, AIBinder_DeathRecipient *recipient, void *cookie);
binder_status_t AIBinder_unlinkToDeath(AIBinder *binder, AIBinder_DeathRecipient *recipient, void *cookie);

// =============================================================================
// Weak References
// =============================================================================

AIBinder_Weak *AIBinder_Weak_new(AIBinder *binder);
void AIBinder_Weak_delete(AIBinder_Weak *weakBinder);
AIBinder *AIBinder_Weak_promote(AIBinder_Weak *weakBinder);
AIBinder_Weak *AIBinder_Weak_clone(const AIBinder_Weak *weak);

// =============================================================================
// Extensions
// =============================================================================

binder_status_t AIBinder_getExtension(AIBinder *binder, AIBinder **outExt);
binder_status_t AIBinder_setExtension(AIBinder *binder, AIBinder *ext);

// =============================================================================
// Diagnostics
// =============================================================================

binder_status_t AIBinder_ping(AIBinder *binder);

// =============================================================================
// Parcel Core
// =============================================================================

void AParcel_delete(AParcel *parcel);
int32_t AParcel_getDataPosition(const AParcel *parcel);
binder_status_t AParcel_setDataPosition(const AParcel *parcel, int32_t position);
int32_t AParcel_getDataSize(const AParcel *parcel);
AParcel *AParcel_create();
binder_status_t AParcel_appendFrom(const AParcel *from, AParcel *to, int32_t start, int32_t size);
void AParcel_markSensitive(const AParcel *parcel);
void AParcel_reset(AParcel *parcel);
binder_status_t AParcel_marshal(const AParcel *parcel, uint8_t *buffer, size_t start, size_t size);
binder_status_t AParcel_unmarshal(AParcel *parcel, const uint8_t *buffer, size_t size);

// =============================================================================
// Parcel Binder & Descriptors
// =============================================================================

binder_status_t AParcel_writeStrongBinder(AParcel *parcel, AIBinder *binder);
binder_status_t AParcel_readStrongBinder(const AParcel *parcel, AIBinder **binder);
binder_status_t AParcel_writeStatusHeader(AParcel *parcel, const AStatus *status);
binder_status_t AParcel_readStatusHeader(const AParcel *parcel, AStatus **status);
binder_status_t AParcel_writeParcelFileDescriptor(AParcel *parcel, int fd);
binder_status_t AParcel_readParcelFileDescriptor(const AParcel *parcel, int *fd);
binder_status_t AParcel_writeInterfaceToken(AParcel *parcel, const char *interface);

// =============================================================================
// Parcel Single Values
// =============================================================================

binder_status_t AParcel_writeString(AParcel *parcel, const char *string, int32_t length);
binder_status_t AParcel_readString(const AParcel *parcel, void *stringData, bool (*allocator)(void *, int32_t, char **));
binder_status_t AParcel_writeInt32(AParcel *parcel, int32_t value);
binder_status_t AParcel_readInt32(const AParcel *parcel, int32_t *value);
binder_status_t AParcel_writeUint32(AParcel *parcel, uint32_t value);
binder_status_t AParcel_readUint32(const AParcel *parcel, uint32_t *value);
binder_status_t AParcel_writeInt64(AParcel *parcel, int64_t value);
binder_status_t AParcel_readInt64(const AParcel *parcel, int64_t *value);
binder_status_t AParcel_writeUint64(AParcel *parcel, uint64_t value);
binder_status_t AParcel_readUint64(const AParcel *parcel, uint64_t *value);
binder_status_t AParcel_writeFloat(AParcel *parcel, float value);
binder_status_t AParcel_readFloat(const AParcel *parcel, float *value);
binder_status_t AParcel_writeDouble(AParcel *parcel, double value);
binder_status_t AParcel_readDouble(const AParcel *parcel, double *value);
binder_status_t AParcel_writeBool(AParcel *parcel, bool value);
binder_status_t AParcel_readBool(const AParcel *parcel, bool *value);
binder_status_t AParcel_writeByte(AParcel *parcel, int8_t value);
binder_status_t AParcel_readByte(const AParcel *parcel, int8_t *value);
binder_status_t AParcel_writeChar(AParcel *parcel, char16_t value);
binder_status_t AParcel_readChar(const AParcel *parcel, char16_t *value);

// =============================================================================
// Parcel Arrays
// =============================================================================

binder_status_t AParcel_writeBoolArray(AParcel *parcel, const bool *array, int32_t length);
binder_status_t AParcel_readBoolArray(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t, bool **));
binder_status_t AParcel_writeByteArray(AParcel *parcel, const int8_t *array, int32_t length);
binder_status_t AParcel_readByteArray(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t, int8_t **));
binder_status_t AParcel_writeCharArray(AParcel *parcel, const char16_t *array, int32_t length);
binder_status_t AParcel_readCharArray(const AParcel *parcel, void *arrayData, AParcel_charArrayAllocator allocator);
binder_status_t AParcel_writeInt32Array(AParcel *parcel, const int32_t *array, int32_t length);
binder_status_t AParcel_readInt32Array(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t, int32_t **));
binder_status_t AParcel_writeUint32Array(AParcel *parcel, const uint32_t *array, int32_t length);
binder_status_t AParcel_readUint32Array(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t, uint32_t **));
binder_status_t AParcel_writeInt64Array(AParcel *parcel, const int64_t *array, int32_t length);
binder_status_t AParcel_readInt64Array(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t, int64_t **));
binder_status_t AParcel_writeUint64Array(AParcel *parcel, const uint64_t *array, int32_t length);
binder_status_t AParcel_readUint64Array(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t, uint64_t **));
binder_status_t AParcel_writeFloatArray(AParcel *parcel, const float *array, int32_t length);
binder_status_t AParcel_readFloatArray(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t, float **));
binder_status_t AParcel_writeDoubleArray(AParcel *parcel, const double *array, int32_t length);
binder_status_t AParcel_readDoubleArray(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t, double **));
binder_status_t AParcel_writeStringArray(AParcel *parcel, const char *const *array, int32_t length);
binder_status_t AParcel_readStringArray(const AParcel *parcel, void *arrayData, AParcel_stringArrayAllocator allocator);
binder_status_t AParcel_writeParcelableArray(AParcel *parcel, const void *arrayData, int32_t length, binder_status_t (*elementWriter)(AParcel *, const void *, size_t));
binder_status_t AParcel_readParcelableArray(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t), binder_status_t (*elementReader)(const AParcel *, void *, size_t));

// =============================================================================
// AStatus
// =============================================================================

AStatus *AStatus_newOk();
AStatus *AStatus_fromExceptionCode(binder_exception_t exception);
AStatus *AStatus_fromExceptionCodeWithMessage(binder_exception_t exception, const char *message);
AStatus *AStatus_fromServiceSpecificError(int32_t serviceSpecific);
AStatus *AStatus_fromServiceSpecificErrorWithMessage(int32_t serviceSpecific, const char *message);
AStatus *AStatus_fromStatus(binder_status_t status);
bool AStatus_isOk(const AStatus *status);
binder_exception_t AStatus_getExceptionCode(const AStatus *status);
int32_t AStatus_getServiceSpecificError(const AStatus *status);
binder_status_t AStatus_getStatus(const AStatus *status);
const char *AStatus_getMessage(const AStatus *status);
const char *AStatus_getDescription(const AStatus *status);
void AStatus_deleteDescription(const char *description);
void AStatus_delete(AStatus *status);

} // extern "C"
