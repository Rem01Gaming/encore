#pragma once

#include "JniHelper.hpp"

namespace jni::zygisk {
    /**
     * @brief Process wide holder for the JavaVM pointer.
     *
     * Zygisk hook callbacks fire on threads the JVM never handed you a JNIEnv for, so there is
     * no env to thread through. Call Init() once with the JavaVM* (e.g. from JNI_OnLoad or the
     * first callback that does have one), then any code can reach the VM via Get().
     */
    class VM {
    public:
        static void Init(JavaVM* vm) { vm_ = vm; }
        static JavaVM* Get() { return vm_; }

    private:
        static inline JavaVM* vm_ = nullptr;
    };

    /**
     * @brief RAII guard that attaches the current thread to the JVM only if it is not already
     *        attached, and detaches it only if this guard performed the attach.
     *
     * Use this from any thread that might not be JVM attached (inline hooks, syscall
     * interceptors, companion threads). env() returns nullptr if attaching failed.
     */
    class ScopedThreadAttach {
    public:
        explicit ScopedThreadAttach(JavaVM* vm) : vm_(vm) {
            if (!vm_) return;

            jint status = vm_->GetEnv(reinterpret_cast<void**>(&env_), JNI_VERSION_1_6);
            if (status == JNI_EDETACHED) {
                if (vm_->AttachCurrentThread(&env_, nullptr) == JNI_OK) {
                    attached_ = true;
                } else {
                    env_ = nullptr;
                }
            } else if (status != JNI_OK) {
                env_ = nullptr;
            }
        }

        ~ScopedThreadAttach() {
            if (attached_) vm_->DetachCurrentThread();
        }

        JNIEnv* env() const { return env_; }

        ScopedThreadAttach(const ScopedThreadAttach&) = delete;
        ScopedThreadAttach& operator=(const ScopedThreadAttach&) = delete;

    private:
        JavaVM* vm_;
        JNIEnv* env_ = nullptr;
        bool attached_ = false;
    };

    /**
     * @brief RAII wrapper for a JNI global reference that can be released from any thread.
     *
     * Unlike jni::ScopedGlobalRef, this does not need an env at destruction time; it pulls one
     * from VM::Get() via ScopedThreadAttach, since global refs cached by a hook module commonly
     * outlive the call that created them and get released from an arbitrary thread later on.
     * Requires VM::Init() to have been called, otherwise the ref is leaked instead of crashing.
     */
    template <typename T>
    class ScopedGlobalRef {
    public:
        ScopedGlobalRef() = default;

        // Wraps localRef as a new global ref and deletes localRef, since the caller almost
        // always owns it and would otherwise leak it.
        ScopedGlobalRef(JNIEnv* env, T localRef) {
            if (localRef) {
                ref_ = static_cast<T>(env->NewGlobalRef(localRef));
                env->DeleteLocalRef(localRef);
            }
        }

        ~ScopedGlobalRef() {
            if (ref_ && VM::Get()) {
                ScopedThreadAttach guard(VM::Get());
                if (guard.env()) guard.env()->DeleteGlobalRef(ref_);
            }
        }

        T get() const { return ref_; }

        T release() {
            T temp = ref_;
            ref_ = nullptr;
            return temp;
        }

        ScopedGlobalRef(const ScopedGlobalRef&) = delete;
        ScopedGlobalRef& operator=(const ScopedGlobalRef&) = delete;

        ScopedGlobalRef(ScopedGlobalRef&& other) noexcept : ref_(other.ref_) { other.ref_ = nullptr; }

        ScopedGlobalRef& operator=(ScopedGlobalRef&& other) noexcept {
            if (this != &other) {
                ref_ = other.ref_;
                other.ref_ = nullptr;
            }
            return *this;
        }

    private:
        T ref_ = nullptr;
    };

    /**
     * @brief Caches the app's ClassLoader so app defined classes can still be resolved from
     *        threads the JVM never attached (env->FindClass() there only sees the bootstrap
     *        classloader and fails for anything app defined).
     */
    class ClassLoaderCache {
    public:
        // anyAppObject is any live instance of an app defined class, used only to reach its
        // classloader. Must be called once from a normal, JVM attached thread.
        static void Init(JNIEnv* env, jobject anyAppObject) {
            jclass objClass = env->GetObjectClass(anyAppObject);
            ScopedLocalRef<jclass> objClassRef(env, objClass);

            jmethodID getClassLoader = GetMethodID(env, objClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
            jobject loader = env->CallObjectMethod(anyAppObject, getClassLoader);
            JNI_CHECK_EXCEPTION(env);

            classLoader_ = ScopedGlobalRef<jobject>(env, loader);

            jclass loaderClass = env->GetObjectClass(classLoader_.get());
            ScopedLocalRef<jclass> loaderClassRef(env, loaderClass);
            loadClassMethod_ = GetMethodID(env, loaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
        }

        static bool IsInitialized() { return classLoader_.get() != nullptr; }

        static jclass FindClass(JNIEnv* env, const char* className) {
            std::string dotName(className);
            for (char& c : dotName) if (c == '/') c = '.';

            jstring jname = StringToJString(env, dotName);
            ScopedLocalRef<jstring> jnameRef(env, jname);

            jobject cls = env->CallObjectMethod(classLoader_.get(), loadClassMethod_, jname);
            JNI_CHECK_EXCEPTION(env);

            return static_cast<jclass>(cls);
        }

    private:
        static inline ScopedGlobalRef<jobject> classLoader_;
        static inline jmethodID loadClassMethod_ = nullptr;
    };

    /**
     * @brief Resolves a class by name, falling back to the cached app ClassLoader when the
     *        plain JNI FindClass fails.
     *
     * Call ClassLoaderCache::Init() once beforehand for the fallback to work; otherwise this
     * behaves exactly like FindClass().
     */
    inline jclass FindClassSafe(JNIEnv* env, const char* className) {
        jclass cls = env->FindClass(className);
        if (cls != nullptr) return cls;

        env->ExceptionClear();

        if (ClassLoaderCache::IsInitialized()) {
            return ClassLoaderCache::FindClass(env, className);
        }

        throw JNIException("FindClass failed and no ClassLoader cache available");
    }
} // namespace jni::zygisk
