#pragma once

#include <jni.h>

#include <cstdint>
#include <exception>
#include <string>
#include <utility>
#include <vector>

#include "JniHelper.hpp"

namespace dexloader {

    namespace detail {

        inline void ClearPendingException(JNIEnv* env) noexcept {
        if (env != nullptr && env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

/**
 * Creates a dalvik.system.InMemoryDexClassLoader from raw DEX bytes.
 *
 * Returns a JNI local reference to the ClassLoader.
 */
inline jobject CreateInMemoryClassLoader(JNIEnv* env,
                                         const std::vector<uint8_t>& dexBytes) {
    if (env == nullptr) {
        throw jni::JNIException("JNIEnv is null");
    }

    if (dexBytes.empty()) {
        throw jni::JNIException("DEX payload is empty");
    }

    jclass classLoaderClass = jni::FindClass(env, "java/lang/ClassLoader");
    if (classLoaderClass == nullptr) {
        throw jni::JNIException("java/lang/ClassLoader not found");
    }
    jni::ScopedLocalRef<jclass> classLoaderClassRef(env, classLoaderClass);

    jmethodID getSystemClassLoader = jni::GetStaticMethodID(
            env,
            classLoaderClass,
            "getSystemClassLoader",
            "()Ljava/lang/ClassLoader;"
    );

    jobject systemClassLoader = env->CallStaticObjectMethod(
            classLoaderClass,
            getSystemClassLoader
    );
    JNI_CHECK_EXCEPTION(env);

    if (systemClassLoader == nullptr) {
        throw jni::JNIException("getSystemClassLoader returned null");
    }
    jni::ScopedLocalRef<jobject> systemClassLoaderRef(env, systemClassLoader);

    void* address = const_cast<void*>(static_cast<const void*>(dexBytes.data()));
    jobject dexBuffer = env->NewDirectByteBuffer(
            address,
            static_cast<jlong>(dexBytes.size())
    );
    JNI_CHECK_EXCEPTION(env);

    if (dexBuffer == nullptr) {
        throw jni::JNIException("NewDirectByteBuffer returned null");
    }
    jni::ScopedLocalRef<jobject> dexBufferRef(env, dexBuffer);

    jclass inMemoryDexClassLoaderClass = jni::FindClass(
            env,
            "dalvik/system/InMemoryDexClassLoader"
    );
    if (inMemoryDexClassLoaderClass == nullptr) {
        throw jni::JNIException("dalvik/system/InMemoryDexClassLoader not found");
    }
    jni::ScopedLocalRef<jclass> inMemoryDexClassLoaderClassRef(
            env,
            inMemoryDexClassLoaderClass
    );

    jmethodID inMemoryDexClassLoaderInit = jni::GetMethodID(
            env,
            inMemoryDexClassLoaderClass,
            "<init>",
            "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V"
    );

    jobject dexClassLoader = env->NewObject(
            inMemoryDexClassLoaderClass,
            inMemoryDexClassLoaderInit,
            dexBuffer,
            systemClassLoader
    );
    JNI_CHECK_EXCEPTION(env);

    if (dexClassLoader == nullptr) {
        throw jni::JNIException("Failed to create InMemoryDexClassLoader");
    }

    return dexClassLoader;
}

/**
 * Loads a class from an existing ClassLoader.
 *
 * Returns a JNI local reference to the loaded jclass.
 */
inline jclass LoadClassWithClassLoader(JNIEnv* env,
                                       jobject classLoader,
                                       const std::string& className) {
    if (env == nullptr) {
        throw jni::JNIException("JNIEnv is null");
    }

    if (classLoader == nullptr) {
        throw jni::JNIException("ClassLoader is null");
    }

    if (className.empty()) {
        throw jni::JNIException("Class name is empty");
    }

    jclass classLoaderClass = env->GetObjectClass(classLoader);
    JNI_CHECK_EXCEPTION(env);

    if (classLoaderClass == nullptr) {
        throw jni::JNIException("GetObjectClass(ClassLoader) failed");
    }
    jni::ScopedLocalRef<jclass> classLoaderClassRef(env, classLoaderClass);

    jmethodID loadClass = jni::GetMethodID(
            env,
            classLoaderClass,
            "loadClass",
            "(Ljava/lang/String;)Ljava/lang/Class;"
    );

    jstring jClassName = jni::StringToJString(env, className);
    JNI_CHECK_EXCEPTION(env);

    if (jClassName == nullptr) {
        throw jni::JNIException("Failed to create class name string");
    }
    jni::ScopedLocalRef<jstring> jClassNameRef(env, jClassName);

    jobject loadedClass = env->CallObjectMethod(
            classLoader,
            loadClass,
            jClassName
    );
    JNI_CHECK_EXCEPTION(env);

    if (loadedClass == nullptr) {
        throw jni::JNIException("ClassLoader.loadClass returned null");
    }

    return static_cast<jclass>(loadedClass);
}

} // namespace detail

/**
 * One-shot in-memory DEX loader.
 *
 * This is the direct replacement for the current injectDex() logic.
 */
class InMemoryDexLoader {
public:
    struct Config {
        std::string entryClassName;

        // Default matches PlayIntegrityFix EntryPoint:
        // static void init(String json, boolean spoofProvider, boolean spoofSignature, boolean spoofBuild)
        std::string initMethodName = "init";
        std::string initSignature = "(Ljava/lang/String;ZZZ)V";
    };

    explicit InMemoryDexLoader(Config config) : config_(std::move(config)) {}

    const Config& config() const {
        return config_;
    }

    /**
     * Loads the DEX, loads the entry class, and calls:
     *
     *   EntryPoint.init(String json, boolean flag1, boolean flag2, boolean flag3)
     */
    bool Inject(JNIEnv* env,
                const std::vector<uint8_t>& dexBytes,
                const std::string& jsonConfig,
                bool flag1,
                bool flag2,
                bool flag3,
                std::string* error) const {
        try {
            InjectOrThrow(env, dexBytes, jsonConfig, flag1, flag2, flag3);
            return true;
        } catch (const std::exception& e) {
            detail::ClearPendingException(env);
            if (error != nullptr) {
                *error = e.what();
            }
            return false;
        } catch (...) {
            detail::ClearPendingException(env);
            if (error != nullptr) {
                *error = "Unknown DEX loader error";
            }
            return false;
        }
    }

    /**
     * Loads the entry class and returns JNI local references.
     *
     * Caller owns:
     *   *outEntryClass
     *   *outClassLoader, if outClassLoader is non-null
     *
     * Delete them with DeleteLocalRef when done.
     */
    bool LoadEntryPoint(JNIEnv* env,
                        const std::vector<uint8_t>& dexBytes,
                        jclass* outEntryClass,
                        jobject* outClassLoader,
                        std::string* error) const {
        if (outEntryClass == nullptr) {
            if (error != nullptr) {
                *error = "outEntryClass is null";
            }
            return false;
        }

        try {
            if (env == nullptr) {
                throw jni::JNIException("JNIEnv is null");
            }

            if (config_.entryClassName.empty()) {
                throw jni::JNIException("entryClassName is empty");
            }

            jobject classLoaderLocal = detail::CreateInMemoryClassLoader(env, dexBytes);
            jni::ScopedLocalRef<jobject> classLoaderRef(env, classLoaderLocal);

            jclass entryClassLocal = detail::LoadClassWithClassLoader(
                    env,
                    classLoaderRef.get(),
                    config_.entryClassName
            );

            if (outClassLoader != nullptr) {
                *outClassLoader = classLoaderRef.release();
            }

            *outEntryClass = entryClassLocal;
            return true;
        } catch (const std::exception& e) {
            detail::ClearPendingException(env);
            if (error != nullptr) {
                *error = e.what();
            }
            return false;
        } catch (...) {
            detail::ClearPendingException(env);
            if (error != nullptr) {
                *error = "Unknown DEX loader error";
            }
            return false;
        }
    }

    /**
     * Generic escape hatch for other projects.
     *
     * Invoker signature:
     *
     *   void(JNIEnv* env, jclass entryClass, jobject classLoader)
     */
    template <typename Invoker>
    bool LoadAndInvoke(JNIEnv* env,
                       const std::vector<uint8_t>& dexBytes,
                       Invoker&& invoker,
                       std::string* error) const {
        try {
            if (env == nullptr) {
                throw jni::JNIException("JNIEnv is null");
            }

            if (config_.entryClassName.empty()) {
                throw jni::JNIException("entryClassName is empty");
            }

            jobject classLoaderLocal = detail::CreateInMemoryClassLoader(env, dexBytes);
            jni::ScopedLocalRef<jobject> classLoaderRef(env, classLoaderLocal);

            jclass entryClassLocal = detail::LoadClassWithClassLoader(
                    env,
                    classLoaderRef.get(),
                    config_.entryClassName
            );
            jni::ScopedLocalRef<jclass> entryClassRef(env, entryClassLocal);

            std::forward<Invoker>(invoker)(
                    env,
                    entryClassRef.get(),
                    classLoaderRef.get()
            );

            return true;
        } catch (const std::exception& e) {
            detail::ClearPendingException(env);
            if (error != nullptr) {
                *error = e.what();
            }
            return false;
        } catch (...) {
            detail::ClearPendingException(env);
            if (error != nullptr) {
                *error = "Unknown DEX loader error";
            }
            return false;
        }
    }

private:
    void InjectOrThrow(JNIEnv* env,
                       const std::vector<uint8_t>& dexBytes,
                       const std::string& jsonConfig,
                       bool flag1,
                       bool flag2,
                       bool flag3) const {
        if (env == nullptr) {
            throw jni::JNIException("JNIEnv is null");
        }

        if (config_.entryClassName.empty()) {
            throw jni::JNIException("entryClassName is empty");
        }

        if (config_.initMethodName.empty()) {
            throw jni::JNIException("initMethodName is empty");
        }

        if (config_.initSignature.empty()) {
            throw jni::JNIException("initSignature is empty");
        }

        jobject classLoaderLocal = detail::CreateInMemoryClassLoader(env, dexBytes);
        jni::ScopedLocalRef<jobject> classLoaderRef(env, classLoaderLocal);

        jclass entryClassLocal = detail::LoadClassWithClassLoader(
                env,
                classLoaderRef.get(),
                config_.entryClassName
        );
        jni::ScopedLocalRef<jclass> entryClassRef(env, entryClassLocal);

        jmethodID entryInit = jni::GetStaticMethodID(
                env,
                entryClassRef.get(),
                config_.initMethodName.c_str(),
                config_.initSignature.c_str()
        );

        jstring json = jni::StringToJString(env, jsonConfig);
        JNI_CHECK_EXCEPTION(env);

        if (json == nullptr) {
            throw jni::JNIException("Failed to create JSON string");
        }
        jni::ScopedLocalRef<jstring> jsonRef(env, json);

        env->CallStaticVoidMethod(
                entryClassRef.get(),
                entryInit,
                jsonRef.get(),
                static_cast<jboolean>(flag1 ? JNI_TRUE : JNI_FALSE),
                static_cast<jboolean>(flag2 ? JNI_TRUE : JNI_FALSE),
                static_cast<jboolean>(flag3 ? JNI_TRUE : JNI_FALSE)
        );
        JNI_CHECK_EXCEPTION(env);
    }

    Config config_;
};

} // namespace dexloader
