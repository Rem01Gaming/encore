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

#include "BinderNDK.hpp"

#include <dlfcn.h>
#include <mutex>

namespace {

    void *g_libbinder_handle = nullptr;
    std::once_flag g_init_flag;

#define DEFINE_PTR(name) decltype(&::name) p_##name = nullptr

    DEFINE_PTR(AServiceManager_getService);
    DEFINE_PTR(AServiceManager_waitForService);
    DEFINE_PTR(AServiceManager_addService);
    DEFINE_PTR(AServiceManager_checkService);
    DEFINE_PTR(AServiceManager_isDeclared);
    DEFINE_PTR(AServiceManager_forEachDeclaredInstance);
    DEFINE_PTR(AIBinder_Class_define);
    DEFINE_PTR(AIBinder_Class_getDescriptor);
    DEFINE_PTR(AIBinder_Class_disableInterfaceTokenHeader);
    DEFINE_PTR(AIBinder_new);
    DEFINE_PTR(AIBinder_associateClass);
    DEFINE_PTR(AIBinder_incStrong);
    DEFINE_PTR(AIBinder_decStrong);
    DEFINE_PTR(AIBinder_getClass);
    DEFINE_PTR(AIBinder_getUserData);
    DEFINE_PTR(AIBinder_isAlive);
    DEFINE_PTR(AIBinder_isRemote);
    DEFINE_PTR(AIBinder_isHandlingTransaction);
    DEFINE_PTR(AIBinder_getCallingUid);
    DEFINE_PTR(AIBinder_getCallingPid);
    DEFINE_PTR(AIBinder_getCallingSid);
    DEFINE_PTR(ABinderProcess_startThreadPool);
    DEFINE_PTR(ABinderProcess_joinThreadPool);
    DEFINE_PTR(ABinderProcess_setThreadPoolMaxThreadCount);
    DEFINE_PTR(ABinderProcess_isThreadPoolStarted);
    DEFINE_PTR(AIBinder_prepareTransaction);
    DEFINE_PTR(AIBinder_transact);
    DEFINE_PTR(AIBinder_DeathRecipient_new);
    DEFINE_PTR(AIBinder_DeathRecipient_delete);
    DEFINE_PTR(AIBinder_DeathRecipient_setOnUnlinked);
    DEFINE_PTR(AIBinder_linkToDeath);
    DEFINE_PTR(AIBinder_unlinkToDeath);
    DEFINE_PTR(AIBinder_Weak_new);
    DEFINE_PTR(AIBinder_Weak_delete);
    DEFINE_PTR(AIBinder_Weak_promote);
    DEFINE_PTR(AIBinder_Weak_clone);
    DEFINE_PTR(AIBinder_getExtension);
    DEFINE_PTR(AIBinder_setExtension);
    DEFINE_PTR(AIBinder_ping);
    DEFINE_PTR(AParcel_delete);
    DEFINE_PTR(AParcel_getDataPosition);
    DEFINE_PTR(AParcel_setDataPosition);
    DEFINE_PTR(AParcel_getDataSize);
    DEFINE_PTR(AParcel_create);
    DEFINE_PTR(AParcel_appendFrom);
    DEFINE_PTR(AParcel_markSensitive);
    DEFINE_PTR(AParcel_reset);
    DEFINE_PTR(AParcel_marshal);
    DEFINE_PTR(AParcel_unmarshal);
    DEFINE_PTR(AParcel_writeStrongBinder);
    DEFINE_PTR(AParcel_readStrongBinder);
    DEFINE_PTR(AParcel_writeStatusHeader);
    DEFINE_PTR(AParcel_readStatusHeader);
    DEFINE_PTR(AParcel_writeParcelFileDescriptor);
    DEFINE_PTR(AParcel_readParcelFileDescriptor);
    DEFINE_PTR(AParcel_writeInterfaceToken);
    DEFINE_PTR(AParcel_writeString);
    DEFINE_PTR(AParcel_readString);
    DEFINE_PTR(AParcel_writeInt32);
    DEFINE_PTR(AParcel_readInt32);
    DEFINE_PTR(AParcel_writeUint32);
    DEFINE_PTR(AParcel_readUint32);
    DEFINE_PTR(AParcel_writeInt64);
    DEFINE_PTR(AParcel_readInt64);
    DEFINE_PTR(AParcel_writeUint64);
    DEFINE_PTR(AParcel_readUint64);
    DEFINE_PTR(AParcel_writeFloat);
    DEFINE_PTR(AParcel_readFloat);
    DEFINE_PTR(AParcel_writeDouble);
    DEFINE_PTR(AParcel_readDouble);
    DEFINE_PTR(AParcel_writeBool);
    DEFINE_PTR(AParcel_readBool);
    DEFINE_PTR(AParcel_writeByte);
    DEFINE_PTR(AParcel_readByte);
    DEFINE_PTR(AParcel_writeChar);
    DEFINE_PTR(AParcel_readChar);
    DEFINE_PTR(AParcel_writeBoolArray);
    DEFINE_PTR(AParcel_readBoolArray);
    DEFINE_PTR(AParcel_writeByteArray);
    DEFINE_PTR(AParcel_readByteArray);
    DEFINE_PTR(AParcel_writeCharArray);
    DEFINE_PTR(AParcel_readCharArray);
    DEFINE_PTR(AParcel_writeInt32Array);
    DEFINE_PTR(AParcel_readInt32Array);
    DEFINE_PTR(AParcel_writeUint32Array);
    DEFINE_PTR(AParcel_readUint32Array);
    DEFINE_PTR(AParcel_writeInt64Array);
    DEFINE_PTR(AParcel_readInt64Array);
    DEFINE_PTR(AParcel_writeUint64Array);
    DEFINE_PTR(AParcel_readUint64Array);
    DEFINE_PTR(AParcel_writeFloatArray);
    DEFINE_PTR(AParcel_readFloatArray);
    DEFINE_PTR(AParcel_writeDoubleArray);
    DEFINE_PTR(AParcel_readDoubleArray);
    DEFINE_PTR(AParcel_writeStringArray);
    DEFINE_PTR(AParcel_readStringArray);
    DEFINE_PTR(AParcel_writeParcelableArray);
    DEFINE_PTR(AParcel_readParcelableArray);
    DEFINE_PTR(AStatus_newOk);
    DEFINE_PTR(AStatus_fromExceptionCode);
    DEFINE_PTR(AStatus_fromExceptionCodeWithMessage);
    DEFINE_PTR(AStatus_fromServiceSpecificError);
    DEFINE_PTR(AStatus_fromServiceSpecificErrorWithMessage);
    DEFINE_PTR(AStatus_fromStatus);
    DEFINE_PTR(AStatus_isOk);
    DEFINE_PTR(AStatus_getExceptionCode);
    DEFINE_PTR(AStatus_getServiceSpecificError);
    DEFINE_PTR(AStatus_getStatus);
    DEFINE_PTR(AStatus_getMessage);
    DEFINE_PTR(AStatus_getDescription);
    DEFINE_PTR(AStatus_deleteDescription);
    DEFINE_PTR(AStatus_delete);

#undef DEFINE_PTR

    void load_binder_ndk() {
        g_libbinder_handle = dlopen("libbinder_ndk.so", RTLD_NOW | RTLD_LOCAL);
        if (!g_libbinder_handle) return;

#define LOAD(name) p_##name = reinterpret_cast<decltype(p_##name)>(dlsym(g_libbinder_handle, #name))

        LOAD(AServiceManager_getService);
        LOAD(AServiceManager_waitForService);
        LOAD(AServiceManager_addService);
        LOAD(AServiceManager_checkService);
        LOAD(AServiceManager_isDeclared);
        LOAD(AServiceManager_forEachDeclaredInstance);
        LOAD(AIBinder_Class_define);
        LOAD(AIBinder_Class_getDescriptor);
        LOAD(AIBinder_Class_disableInterfaceTokenHeader);
        LOAD(AIBinder_new);
        LOAD(AIBinder_associateClass);
        LOAD(AIBinder_incStrong);
        LOAD(AIBinder_decStrong);
        LOAD(AIBinder_getClass);
        LOAD(AIBinder_getUserData);
        LOAD(AIBinder_isAlive);
        LOAD(AIBinder_isRemote);
        LOAD(AIBinder_isHandlingTransaction);
        LOAD(AIBinder_getCallingUid);
        LOAD(AIBinder_getCallingPid);
        LOAD(AIBinder_getCallingSid);
        LOAD(ABinderProcess_startThreadPool);
        LOAD(ABinderProcess_joinThreadPool);
        LOAD(ABinderProcess_setThreadPoolMaxThreadCount);
        LOAD(ABinderProcess_isThreadPoolStarted);
        LOAD(AIBinder_prepareTransaction);
        LOAD(AIBinder_transact);
        LOAD(AIBinder_DeathRecipient_new);
        LOAD(AIBinder_DeathRecipient_delete);
        LOAD(AIBinder_DeathRecipient_setOnUnlinked);
        LOAD(AIBinder_linkToDeath);
        LOAD(AIBinder_unlinkToDeath);
        LOAD(AIBinder_Weak_new);
        LOAD(AIBinder_Weak_delete);
        LOAD(AIBinder_Weak_promote);
        LOAD(AIBinder_Weak_clone);
        LOAD(AIBinder_getExtension);
        LOAD(AIBinder_setExtension);
        LOAD(AIBinder_ping);
        LOAD(AParcel_delete);
        LOAD(AParcel_getDataPosition);
        LOAD(AParcel_setDataPosition);
        LOAD(AParcel_getDataSize);
        LOAD(AParcel_create);
        LOAD(AParcel_appendFrom);
        LOAD(AParcel_markSensitive);
        LOAD(AParcel_reset);
        LOAD(AParcel_marshal);
        LOAD(AParcel_unmarshal);
        LOAD(AParcel_writeStrongBinder);
        LOAD(AParcel_readStrongBinder);
        LOAD(AParcel_writeStatusHeader);
        LOAD(AParcel_readStatusHeader);
        LOAD(AParcel_writeParcelFileDescriptor);
        LOAD(AParcel_readParcelFileDescriptor);
        LOAD(AParcel_writeInterfaceToken);
        LOAD(AParcel_writeString);
        LOAD(AParcel_readString);
        LOAD(AParcel_writeInt32);
        LOAD(AParcel_readInt32);
        LOAD(AParcel_writeUint32);
        LOAD(AParcel_readUint32);
        LOAD(AParcel_writeInt64);
        LOAD(AParcel_readInt64);
        LOAD(AParcel_writeUint64);
        LOAD(AParcel_readUint64);
        LOAD(AParcel_writeFloat);
        LOAD(AParcel_readFloat);
        LOAD(AParcel_writeDouble);
        LOAD(AParcel_readDouble);
        LOAD(AParcel_writeBool);
        LOAD(AParcel_readBool);
        LOAD(AParcel_writeByte);
        LOAD(AParcel_readByte);
        LOAD(AParcel_writeChar);
        LOAD(AParcel_readChar);
        LOAD(AParcel_writeBoolArray);
        LOAD(AParcel_readBoolArray);
        LOAD(AParcel_writeByteArray);
        LOAD(AParcel_readByteArray);
        LOAD(AParcel_writeCharArray);
        LOAD(AParcel_readCharArray);
        LOAD(AParcel_writeInt32Array);
        LOAD(AParcel_readInt32Array);
        LOAD(AParcel_writeUint32Array);
        LOAD(AParcel_readUint32Array);
        LOAD(AParcel_writeInt64Array);
        LOAD(AParcel_readInt64Array);
        LOAD(AParcel_writeUint64Array);
        LOAD(AParcel_readUint64Array);
        LOAD(AParcel_writeFloatArray);
        LOAD(AParcel_readFloatArray);
        LOAD(AParcel_writeDoubleArray);
        LOAD(AParcel_readDoubleArray);
        LOAD(AParcel_writeStringArray);
        LOAD(AParcel_readStringArray);
        LOAD(AParcel_writeParcelableArray);
        LOAD(AParcel_readParcelableArray);
        LOAD(AStatus_newOk);
        LOAD(AStatus_fromExceptionCode);
        LOAD(AStatus_fromExceptionCodeWithMessage);
        LOAD(AStatus_fromServiceSpecificError);
        LOAD(AStatus_fromServiceSpecificErrorWithMessage);
        LOAD(AStatus_fromStatus);
        LOAD(AStatus_isOk);
        LOAD(AStatus_getExceptionCode);
        LOAD(AStatus_getServiceSpecificError);
        LOAD(AStatus_getStatus);
        LOAD(AStatus_getMessage);
        LOAD(AStatus_getDescription);
        LOAD(AStatus_deleteDescription);
        LOAD(AStatus_delete);

#undef LOAD
    }

#define FORWARD(name, fallback, ...)              \
    std::call_once(g_init_flag, load_binder_ndk); \
    if (!p_##name) return fallback;               \
    return p_##name(__VA_ARGS__)

#define FORWARD_VOID(name, ...)                   \
    std::call_once(g_init_flag, load_binder_ndk); \
    if (p_##name) p_##name(__VA_ARGS__)

} // namespace

extern "C" {

bool BinderNDK_hasSymbol(const char *name) {
    std::call_once(g_init_flag, load_binder_ndk);
    if (!g_libbinder_handle) return false;
    return dlsym(g_libbinder_handle, name) != nullptr;
}

AIBinder *AServiceManager_getService(const char *instance) {
    FORWARD(AServiceManager_getService, nullptr, instance);
}

AIBinder *AServiceManager_waitForService(const char *instance) {
    FORWARD(AServiceManager_waitForService, nullptr, instance);
}

binder_status_t AServiceManager_addService(AIBinder *binder, const char *instance) {
    FORWARD(AServiceManager_addService, STATUS_UNKNOWN_ERROR, binder, instance);
}

AIBinder *AServiceManager_checkService(const char *instance) {
    FORWARD(AServiceManager_checkService, nullptr, instance);
}

bool AServiceManager_isDeclared(const char *instance) {
    FORWARD(AServiceManager_isDeclared, false, instance);
}

void AServiceManager_forEachDeclaredInstance(const char *descriptor, void *context, void (*callback)(const char *, void *)) {
    FORWARD_VOID(AServiceManager_forEachDeclaredInstance, descriptor, context, callback);
}

AIBinder_Class *AIBinder_Class_define(const char *interfaceDescriptor, AIBinder_Class_onCreate onCreate, AIBinder_Class_onDestroy onDestroy, AIBinder_Class_onTransact onTransact) {
    FORWARD(AIBinder_Class_define, nullptr, interfaceDescriptor, onCreate, onDestroy, onTransact);
}

const char *AIBinder_Class_getDescriptor(const AIBinder_Class *clazz) {
    FORWARD(AIBinder_Class_getDescriptor, nullptr, clazz);
}

void AIBinder_Class_disableInterfaceTokenHeader(AIBinder_Class *clazz) {
    FORWARD_VOID(AIBinder_Class_disableInterfaceTokenHeader, clazz);
}

AIBinder *AIBinder_new(const AIBinder_Class *clazz, void *args) {
    FORWARD(AIBinder_new, nullptr, clazz, args);
}

bool AIBinder_associateClass(AIBinder *binder, const AIBinder_Class *clazz) {
    FORWARD(AIBinder_associateClass, false, binder, clazz);
}

void AIBinder_incStrong(AIBinder *binder) {
    FORWARD_VOID(AIBinder_incStrong, binder);
}

void AIBinder_decStrong(AIBinder *binder) {
    FORWARD_VOID(AIBinder_decStrong, binder);
}

const AIBinder_Class *AIBinder_getClass(AIBinder *binder) {
    FORWARD(AIBinder_getClass, nullptr, binder);
}

void *AIBinder_getUserData(AIBinder *binder) {
    FORWARD(AIBinder_getUserData, nullptr, binder);
}

bool AIBinder_isAlive(const AIBinder *binder) {
    FORWARD(AIBinder_isAlive, false, binder);
}

bool AIBinder_isRemote(const AIBinder *binder) {
    FORWARD(AIBinder_isRemote, false, binder);
}

bool AIBinder_isHandlingTransaction() {
    FORWARD(AIBinder_isHandlingTransaction, false);
}

uid_t AIBinder_getCallingUid() {
    FORWARD(AIBinder_getCallingUid, static_cast<uid_t>(0));
}

pid_t AIBinder_getCallingPid() {
    FORWARD(AIBinder_getCallingPid, static_cast<pid_t>(0));
}

const char *AIBinder_getCallingSid() {
    FORWARD(AIBinder_getCallingSid, nullptr);
}

void ABinderProcess_startThreadPool() {
    FORWARD_VOID(ABinderProcess_startThreadPool);
}

void ABinderProcess_joinThreadPool() {
    FORWARD_VOID(ABinderProcess_joinThreadPool);
}

void ABinderProcess_setThreadPoolMaxThreadCount(uint32_t numThreads) {
    FORWARD_VOID(ABinderProcess_setThreadPoolMaxThreadCount, numThreads);
}

bool ABinderProcess_isThreadPoolStarted() {
    FORWARD(ABinderProcess_isThreadPoolStarted, false);
}

binder_status_t AIBinder_prepareTransaction(AIBinder *binder, AParcel **in) {
    FORWARD(AIBinder_prepareTransaction, STATUS_UNKNOWN_ERROR, binder, in);
}

binder_status_t AIBinder_transact(AIBinder *binder, transaction_code_t code, AParcel **in, AParcel **out, binder_flags_t flags) {
    FORWARD(AIBinder_transact, STATUS_UNKNOWN_ERROR, binder, code, in, out, flags);
}

AIBinder_DeathRecipient *AIBinder_DeathRecipient_new(AIBinder_DeathRecipient_onBinderDied onBinderDied) {
    FORWARD(AIBinder_DeathRecipient_new, nullptr, onBinderDied);
}

void AIBinder_DeathRecipient_delete(AIBinder_DeathRecipient *recipient) {
    FORWARD_VOID(AIBinder_DeathRecipient_delete, recipient);
}

void AIBinder_DeathRecipient_setOnUnlinked(AIBinder_DeathRecipient *recipient, void (*onUnlinked)(void *)) {
    FORWARD_VOID(AIBinder_DeathRecipient_setOnUnlinked, recipient, onUnlinked);
}

binder_status_t AIBinder_linkToDeath(AIBinder *binder, AIBinder_DeathRecipient *recipient, void *cookie) {
    FORWARD(AIBinder_linkToDeath, STATUS_UNKNOWN_ERROR, binder, recipient, cookie);
}

binder_status_t AIBinder_unlinkToDeath(AIBinder *binder, AIBinder_DeathRecipient *recipient, void *cookie) {
    FORWARD(AIBinder_unlinkToDeath, STATUS_UNKNOWN_ERROR, binder, recipient, cookie);
}

AIBinder_Weak *AIBinder_Weak_new(AIBinder *binder) {
    FORWARD(AIBinder_Weak_new, nullptr, binder);
}

void AIBinder_Weak_delete(AIBinder_Weak *weakBinder) {
    FORWARD_VOID(AIBinder_Weak_delete, weakBinder);
}

AIBinder *AIBinder_Weak_promote(AIBinder_Weak *weakBinder) {
    FORWARD(AIBinder_Weak_promote, nullptr, weakBinder);
}

AIBinder_Weak *AIBinder_Weak_clone(const AIBinder_Weak *weak) {
    FORWARD(AIBinder_Weak_clone, nullptr, weak);
}

binder_status_t AIBinder_getExtension(AIBinder *binder, AIBinder **outExt) {
    FORWARD(AIBinder_getExtension, STATUS_UNKNOWN_ERROR, binder, outExt);
}

binder_status_t AIBinder_setExtension(AIBinder *binder, AIBinder *ext) {
    FORWARD(AIBinder_setExtension, STATUS_UNKNOWN_ERROR, binder, ext);
}

binder_status_t AIBinder_ping(AIBinder *binder) {
    FORWARD(AIBinder_ping, STATUS_UNKNOWN_ERROR, binder);
}

void AParcel_delete(AParcel *parcel) {
    FORWARD_VOID(AParcel_delete, parcel);
}

int32_t AParcel_getDataPosition(const AParcel *parcel) {
    FORWARD(AParcel_getDataPosition, 0, parcel);
}

binder_status_t AParcel_setDataPosition(const AParcel *parcel, int32_t position) {
    FORWARD(AParcel_setDataPosition, STATUS_UNKNOWN_ERROR, parcel, position);
}

int32_t AParcel_getDataSize(const AParcel *parcel) {
    FORWARD(AParcel_getDataSize, 0, parcel);
}

AParcel *AParcel_create() {
    FORWARD(AParcel_create, nullptr);
}

binder_status_t AParcel_appendFrom(const AParcel *from, AParcel *to, int32_t start, int32_t size) {
    FORWARD(AParcel_appendFrom, STATUS_UNKNOWN_ERROR, from, to, start, size);
}

void AParcel_markSensitive(const AParcel *parcel) {
    FORWARD_VOID(AParcel_markSensitive, parcel);
}

void AParcel_reset(AParcel *parcel) {
    FORWARD_VOID(AParcel_reset, parcel);
}

binder_status_t AParcel_marshal(const AParcel *parcel, uint8_t *buffer, size_t start, size_t size) {
    FORWARD(AParcel_marshal, STATUS_UNKNOWN_ERROR, parcel, buffer, start, size);
}

binder_status_t AParcel_unmarshal(AParcel *parcel, const uint8_t *buffer, size_t size) {
    FORWARD(AParcel_unmarshal, STATUS_UNKNOWN_ERROR, parcel, buffer, size);
}

binder_status_t AParcel_writeStrongBinder(AParcel *parcel, AIBinder *binder) {
    FORWARD(AParcel_writeStrongBinder, STATUS_UNKNOWN_ERROR, parcel, binder);
}

binder_status_t AParcel_readStrongBinder(const AParcel *parcel, AIBinder **binder) {
    FORWARD(AParcel_readStrongBinder, STATUS_UNKNOWN_ERROR, parcel, binder);
}

binder_status_t AParcel_writeStatusHeader(AParcel *parcel, const AStatus *status) {
    FORWARD(AParcel_writeStatusHeader, STATUS_UNKNOWN_ERROR, parcel, status);
}

binder_status_t AParcel_readStatusHeader(const AParcel *parcel, AStatus **status) {
    FORWARD(AParcel_readStatusHeader, STATUS_UNKNOWN_ERROR, parcel, status);
}

binder_status_t AParcel_writeParcelFileDescriptor(AParcel *parcel, int fd) {
    FORWARD(AParcel_writeParcelFileDescriptor, STATUS_UNKNOWN_ERROR, parcel, fd);
}

binder_status_t AParcel_readParcelFileDescriptor(const AParcel *parcel, int *fd) {
    FORWARD(AParcel_readParcelFileDescriptor, STATUS_UNKNOWN_ERROR, parcel, fd);
}

binder_status_t AParcel_writeInterfaceToken(AParcel *parcel, const char *interface) {
    FORWARD(AParcel_writeInterfaceToken, STATUS_UNKNOWN_ERROR, parcel, interface);
}

binder_status_t AParcel_writeString(AParcel *parcel, const char *string, int32_t length) {
    FORWARD(AParcel_writeString, STATUS_UNKNOWN_ERROR, parcel, string, length);
}

binder_status_t AParcel_readString(const AParcel *parcel, void *stringData, bool (*allocator)(void *, int32_t, char **)) {
    FORWARD(AParcel_readString, STATUS_UNKNOWN_ERROR, parcel, stringData, allocator);
}

binder_status_t AParcel_writeInt32(AParcel *parcel, int32_t value) {
    FORWARD(AParcel_writeInt32, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_readInt32(const AParcel *parcel, int32_t *value) {
    FORWARD(AParcel_readInt32, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_writeUint32(AParcel *parcel, uint32_t value) {
    FORWARD(AParcel_writeUint32, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_readUint32(const AParcel *parcel, uint32_t *value) {
    FORWARD(AParcel_readUint32, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_writeInt64(AParcel *parcel, int64_t value) {
    FORWARD(AParcel_writeInt64, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_readInt64(const AParcel *parcel, int64_t *value) {
    FORWARD(AParcel_readInt64, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_writeUint64(AParcel *parcel, uint64_t value) {
    FORWARD(AParcel_writeUint64, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_readUint64(const AParcel *parcel, uint64_t *value) {
    FORWARD(AParcel_readUint64, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_writeFloat(AParcel *parcel, float value) {
    FORWARD(AParcel_writeFloat, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_readFloat(const AParcel *parcel, float *value) {
    FORWARD(AParcel_readFloat, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_writeDouble(AParcel *parcel, double value) {
    FORWARD(AParcel_writeDouble, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_readDouble(const AParcel *parcel, double *value) {
    FORWARD(AParcel_readDouble, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_writeBool(AParcel *parcel, bool value) {
    FORWARD(AParcel_writeBool, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_readBool(const AParcel *parcel, bool *value) {
    FORWARD(AParcel_readBool, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_writeByte(AParcel *parcel, int8_t value) {
    FORWARD(AParcel_writeByte, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_readByte(const AParcel *parcel, int8_t *value) {
    FORWARD(AParcel_readByte, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_writeChar(AParcel *parcel, char16_t value) {
    FORWARD(AParcel_writeChar, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_readChar(const AParcel *parcel, char16_t *value) {
    FORWARD(AParcel_readChar, STATUS_UNKNOWN_ERROR, parcel, value);
}

binder_status_t AParcel_writeBoolArray(AParcel *parcel, const bool *array, int32_t length) {
    FORWARD(AParcel_writeBoolArray, STATUS_UNKNOWN_ERROR, parcel, array, length);
}

binder_status_t AParcel_readBoolArray(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t, bool **)) {
    FORWARD(AParcel_readBoolArray, STATUS_UNKNOWN_ERROR, parcel, arrayData, allocator);
}

binder_status_t AParcel_writeByteArray(AParcel *parcel, const int8_t *array, int32_t length) {
    FORWARD(AParcel_writeByteArray, STATUS_UNKNOWN_ERROR, parcel, array, length);
}

binder_status_t AParcel_readByteArray(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t, int8_t **)) {
    FORWARD(AParcel_readByteArray, STATUS_UNKNOWN_ERROR, parcel, arrayData, allocator);
}

binder_status_t AParcel_writeCharArray(AParcel *parcel, const char16_t *array, int32_t length) {
    FORWARD(AParcel_writeCharArray, STATUS_UNKNOWN_ERROR, parcel, array, length);
}

binder_status_t AParcel_readCharArray(const AParcel *parcel, void *arrayData, AParcel_charArrayAllocator allocator) {
    FORWARD(AParcel_readCharArray, STATUS_UNKNOWN_ERROR, parcel, arrayData, allocator);
}

binder_status_t AParcel_writeInt32Array(AParcel *parcel, const int32_t *array, int32_t length) {
    FORWARD(AParcel_writeInt32Array, STATUS_UNKNOWN_ERROR, parcel, array, length);
}

binder_status_t AParcel_readInt32Array(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t, int32_t **)) {
    FORWARD(AParcel_readInt32Array, STATUS_UNKNOWN_ERROR, parcel, arrayData, allocator);
}

binder_status_t AParcel_writeUint32Array(AParcel *parcel, const uint32_t *array, int32_t length) {
    FORWARD(AParcel_writeUint32Array, STATUS_UNKNOWN_ERROR, parcel, array, length);
}

binder_status_t AParcel_readUint32Array(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t, uint32_t **)) {
    FORWARD(AParcel_readUint32Array, STATUS_UNKNOWN_ERROR, parcel, arrayData, allocator);
}

binder_status_t AParcel_writeInt64Array(AParcel *parcel, const int64_t *array, int32_t length) {
    FORWARD(AParcel_writeInt64Array, STATUS_UNKNOWN_ERROR, parcel, array, length);
}

binder_status_t AParcel_readInt64Array(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t, int64_t **)) {
    FORWARD(AParcel_readInt64Array, STATUS_UNKNOWN_ERROR, parcel, arrayData, allocator);
}

binder_status_t AParcel_writeUint64Array(AParcel *parcel, const uint64_t *array, int32_t length) {
    FORWARD(AParcel_writeUint64Array, STATUS_UNKNOWN_ERROR, parcel, array, length);
}

binder_status_t AParcel_readUint64Array(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t, uint64_t **)) {
    FORWARD(AParcel_readUint64Array, STATUS_UNKNOWN_ERROR, parcel, arrayData, allocator);
}

binder_status_t AParcel_writeFloatArray(AParcel *parcel, const float *array, int32_t length) {
    FORWARD(AParcel_writeFloatArray, STATUS_UNKNOWN_ERROR, parcel, array, length);
}

binder_status_t AParcel_readFloatArray(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t, float **)) {
    FORWARD(AParcel_readFloatArray, STATUS_UNKNOWN_ERROR, parcel, arrayData, allocator);
}

binder_status_t AParcel_writeDoubleArray(AParcel *parcel, const double *array, int32_t length) {
    FORWARD(AParcel_writeDoubleArray, STATUS_UNKNOWN_ERROR, parcel, array, length);
}

binder_status_t AParcel_readDoubleArray(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t, double **)) {
    FORWARD(AParcel_readDoubleArray, STATUS_UNKNOWN_ERROR, parcel, arrayData, allocator);
}

binder_status_t AParcel_writeStringArray(AParcel *parcel, const char *const *array, int32_t length) {
    FORWARD(AParcel_writeStringArray, STATUS_UNKNOWN_ERROR, parcel, array, length);
}

binder_status_t AParcel_readStringArray(const AParcel *parcel, void *arrayData, AParcel_stringArrayAllocator allocator) {
    FORWARD(AParcel_readStringArray, STATUS_UNKNOWN_ERROR, parcel, arrayData, allocator);
}

binder_status_t AParcel_writeParcelableArray(AParcel *parcel, const void *arrayData, int32_t length, binder_status_t (*elementWriter)(AParcel *, const void *, size_t)) {
    FORWARD(AParcel_writeParcelableArray, STATUS_UNKNOWN_ERROR, parcel, arrayData, length, elementWriter);
}

binder_status_t AParcel_readParcelableArray(const AParcel *parcel, void *arrayData, bool (*allocator)(void *, int32_t), binder_status_t (*elementReader)(const AParcel *, void *, size_t)) {
    FORWARD(AParcel_readParcelableArray, STATUS_UNKNOWN_ERROR, parcel, arrayData, allocator, elementReader);
}

AStatus *AStatus_newOk() {
    FORWARD(AStatus_newOk, nullptr);
}

AStatus *AStatus_fromExceptionCode(binder_exception_t exception) {
    FORWARD(AStatus_fromExceptionCode, nullptr, exception);
}

AStatus *AStatus_fromExceptionCodeWithMessage(binder_exception_t exception, const char *message) {
    FORWARD(AStatus_fromExceptionCodeWithMessage, nullptr, exception, message);
}

AStatus *AStatus_fromServiceSpecificError(int32_t serviceSpecific) {
    FORWARD(AStatus_fromServiceSpecificError, nullptr, serviceSpecific);
}

AStatus *AStatus_fromServiceSpecificErrorWithMessage(int32_t serviceSpecific, const char *message) {
    FORWARD(AStatus_fromServiceSpecificErrorWithMessage, nullptr, serviceSpecific, message);
}

AStatus *AStatus_fromStatus(binder_status_t status) {
    FORWARD(AStatus_fromStatus, nullptr, status);
}

bool AStatus_isOk(const AStatus *status) {
    FORWARD(AStatus_isOk, false, status);
}

binder_exception_t AStatus_getExceptionCode(const AStatus *status) {
    FORWARD(AStatus_getExceptionCode, EX_TRANSACTION_FAILED, status);
}

int32_t AStatus_getServiceSpecificError(const AStatus *status) {
    FORWARD(AStatus_getServiceSpecificError, 0, status);
}

binder_status_t AStatus_getStatus(const AStatus *status) {
    FORWARD(AStatus_getStatus, STATUS_UNKNOWN_ERROR, status);
}

const char *AStatus_getMessage(const AStatus *status) {
    FORWARD(AStatus_getMessage, nullptr, status);
}

const char *AStatus_getDescription(const AStatus *status) {
    FORWARD(AStatus_getDescription, nullptr, status);
}

void AStatus_deleteDescription(const char *description) {
    FORWARD_VOID(AStatus_deleteDescription, description);
}

void AStatus_delete(AStatus *status) {
    FORWARD_VOID(AStatus_delete, status);
}

} // extern "C"
