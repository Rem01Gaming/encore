#pragma once

#include <jni.h>
#include <string>
#include <stdexcept>
#include <vector>

namespace jni {
    class JNIException : public std::runtime_error {
    public:
        JNIException(const char* message, jthrowable javaException = nullptr)
                : std::runtime_error(message), javaThrowable(javaException) {}

        jthrowable getJavaException() const { return javaThrowable; }

    private:
        jthrowable javaThrowable;
    };

#define JNI_CHECK_EXCEPTION(env)                                        \
        do {                                                                \
            if ((env)->ExceptionCheck()) {                                  \
                jthrowable exception = (env)->ExceptionOccurred();          \
                (env)->ExceptionDescribe();                                 \
                (env)->ExceptionClear();                                    \
                throw jni::JNIException("JNI exception occurred", exception); \
            }                                                               \
        } while (0)

    template <typename T>
    class ScopedLocalRef {
    public:
        ScopedLocalRef(JNIEnv* env, T ref) : env_(env), ref_(ref) {}

        ~ScopedLocalRef() {
            if (ref_) env_->DeleteLocalRef(ref_);
        }

        T get() const { return ref_; }

        T release() {
            T temp = ref_;
            ref_ = nullptr;
            return temp;
        }

        // Disable copy
        ScopedLocalRef(const ScopedLocalRef&) = delete;
        ScopedLocalRef& operator=(const ScopedLocalRef&) = delete;

    private:
        JNIEnv* env_;
        T ref_;
    };

    /**
     * @brief RAII wrapper for a JNI global reference.
     *
     * Deletes the global ref in its destructor using the JNIEnv it was constructed with, so like
     * ScopedLocalRef it must be destroyed on the same thread it was created on. If you need to
     * release a global ref from a different thread later on (common in Zygisk hook callbacks),
     * see zygisk::ScopedGlobalRef in JniHelperZygisk.hpp instead.
     */
    template <typename T>
    class ScopedGlobalRef {
    public:
        // Wraps localRef as a new global ref and deletes localRef, since the caller almost
        // always owns it and would otherwise leak it.
        ScopedGlobalRef(JNIEnv* env, T localRef) : env_(env) {
            if (localRef) {
                ref_ = static_cast<T>(env->NewGlobalRef(localRef));
                env->DeleteLocalRef(localRef);
            }
        }

        ~ScopedGlobalRef() {
            if (ref_) env_->DeleteGlobalRef(ref_);
        }

        T get() const { return ref_; }

        T release() {
            T temp = ref_;
            ref_ = nullptr;
            return temp;
        }

        // Disable copy
        ScopedGlobalRef(const ScopedGlobalRef&) = delete;
        ScopedGlobalRef& operator=(const ScopedGlobalRef&) = delete;

    private:
        JNIEnv* env_;
        T ref_ = nullptr;
    };

    inline std::string JStringToString(JNIEnv* env, jstring jstr) {
        if (!jstr) return {};

        const char* chars = env->GetStringUTFChars(jstr, nullptr);
        if (!chars) return {};

        std::string result(chars);
        env->ReleaseStringUTFChars(jstr, chars);
        return result;
    }

    inline jstring StringToJString(JNIEnv* env, const std::string& str) {
        return env->NewStringUTF(str.c_str());
    }

    inline jclass FindClass(JNIEnv* env, const char* className) {
        jclass cls = env->FindClass(className);
        JNI_CHECK_EXCEPTION(env);
        return cls;
    }

    inline jmethodID GetMethodID(JNIEnv* env, jclass cls, const char* methodName, const char* signature) {
        jmethodID mid = env->GetMethodID(cls, methodName, signature);
        JNI_CHECK_EXCEPTION(env);
        return mid;
    }

    inline jmethodID GetStaticMethodID(JNIEnv* env, jclass cls, const char* methodName, const char* signature) {
        jmethodID mid = env->GetStaticMethodID(cls, methodName, signature);
        JNI_CHECK_EXCEPTION(env);
        return mid;
    }

    inline jfieldID GetFieldID(JNIEnv* env, jclass cls, const char* fieldName, const char* signature) {
        jfieldID fid = env->GetFieldID(cls, fieldName, signature);
        JNI_CHECK_EXCEPTION(env);
        return fid;
    }

    inline jfieldID GetStaticFieldID(JNIEnv* env, jclass cls, const char* fieldName, const char* signature) {
        jfieldID fid = env->GetStaticFieldID(cls, fieldName, signature);
        JNI_CHECK_EXCEPTION(env);
        return fid;
    }

    template <typename T> struct JNITypeTraits;

    // jobject
    template <> struct JNITypeTraits<jobject> {
        static constexpr const char* signature = "Ljava/lang/Object;";

        // Fields
        static jobject GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jobject result = env->GetObjectField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jobject GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jobject result = env->GetStaticObjectField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Methods
        static jobject CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jobject result = env->CallObjectMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jobject CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jobject result = env->CallStaticObjectMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // jstring
    template <> struct JNITypeTraits<jstring> {
        static constexpr const char* signature = "Ljava/lang/String;";

        // Fields
        static jstring GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jstring result = static_cast<jstring>(env->GetObjectField(obj, fid));
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jstring GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jstring result = static_cast<jstring>(env->GetStaticObjectField(cls, fid));
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Methods
        static jstring CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jstring result = static_cast<jstring>(env->CallObjectMethodA(obj, mid, args));
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jstring CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jstring result = static_cast<jstring>(env->CallStaticObjectMethodA(cls, mid, args));
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // void
    template <> struct JNITypeTraits<void> {
        static constexpr const char* signature = "V";

        // Methods
        static void CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            env->CallVoidMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
        }
        static void CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            env->CallStaticVoidMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
        }
    };

    // jboolean
    template <> struct JNITypeTraits<jboolean> {
        static constexpr const char* signature = "Z";

        // Fields
        static jboolean GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jboolean result = env->GetBooleanField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jboolean GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jboolean result = env->GetStaticBooleanField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Methods
        static jboolean CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jboolean result = env->CallBooleanMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jboolean CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jboolean result = env->CallStaticBooleanMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // jbyte
    template <> struct JNITypeTraits<jbyte> {
        static constexpr const char* signature = "B";

        // Fieldss
        static jbyte GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jbyte result = env->GetByteField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jbyte GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jbyte result = env->GetStaticByteField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Methods
        static jbyte CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jbyte result = env->CallByteMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jbyte CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jbyte result = env->CallStaticByteMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // jchar
    template <> struct JNITypeTraits<jchar> {
        static constexpr const char* signature = "C";

        // Fields
        static jchar GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jchar result = env->GetCharField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jchar GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jchar result = env->GetStaticCharField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Methods
        static jchar CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jchar result = env->CallCharMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jchar CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jchar result = env->CallStaticCharMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // jshort
    template <> struct JNITypeTraits<jshort> {
        static constexpr const char* signature = "S";

        // Fields
        static jshort GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jshort result = env->GetShortField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jshort GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jshort result = env->GetStaticShortField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Methods
        static jshort CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jshort result = env->CallShortMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jshort CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jshort result = env->CallStaticShortMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // jint
    template <> struct JNITypeTraits<jint> {
        static constexpr const char* signature = "I";

        // Fields
        static jint GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jint result = env->GetIntField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jint GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jint result = env->GetStaticIntField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Methods
        static jint CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jint result = env->CallIntMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jint CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jint result = env->CallStaticIntMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // jlong
    template <> struct JNITypeTraits<jlong> {
        static constexpr const char* signature = "J";

        // Fields
        static jlong GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jlong result = env->GetLongField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jlong GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jlong result = env->GetStaticLongField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Methods
        static jlong CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jlong result = env->CallLongMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jlong CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jlong result = env->CallStaticLongMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // jfloat
    template <> struct JNITypeTraits<jfloat> {
        static constexpr const char* signature = "F";

        // Fields
        static jfloat GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jfloat result = env->GetFloatField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jfloat GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jfloat result = env->GetStaticFloatField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Methods
        static jfloat CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jfloat result = env->CallFloatMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jfloat CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jfloat result = env->CallStaticFloatMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // jdouble
    template <> struct JNITypeTraits<jdouble> {
        static constexpr const char* signature = "D";

        // Fields
        static jdouble GetField(JNIEnv* env, jobject obj, jfieldID fid) {
            jdouble result = env->GetDoubleField(obj, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jdouble GetStaticField(JNIEnv* env, jclass cls, jfieldID fid) {
            jdouble result = env->GetStaticDoubleField(cls, fid);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }

        // Methods
        static jdouble CallMethod(JNIEnv* env, jobject obj, jmethodID mid, const jvalue* args) {
            jdouble result = env->CallDoubleMethodA(obj, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
        static jdouble CallStaticMethod(JNIEnv* env, jclass cls, jmethodID mid, const jvalue* args) {
            jdouble result = env->CallStaticDoubleMethodA(cls, mid, args);
            JNI_CHECK_EXCEPTION(env);
            return result;
        }
    };

    // jvalue array
    template <typename... Args>
    class ArgsToJValues {
    public:
        ArgsToJValues(JNIEnv* env, Args... args) {
            convertArgs(env, 0, args...);
        }

        const jvalue* get() const { return values_; }

    private:
        // Make sure we have at least one element in the array
        jvalue values_[sizeof...(Args) > 0 ? sizeof...(Args) : 1];

        template <typename T, typename... RestArgs>
        void convertArgs(JNIEnv* env, int index, T value, RestArgs... rest) {
            setJValue(env, index, value);
            convertArgs(env, index + 1, rest...);
        }

        void convertArgs(JNIEnv*, int) {
            // Base case, no more arguments to convert
        }

        // primitive types
        void setJValue(JNIEnv*, int index, jboolean value) { values_[index].z = value; }
        void setJValue(JNIEnv*, int index, jbyte value) { values_[index].b = value; }
        void setJValue(JNIEnv*, int index, jchar value) { values_[index].c = value; }
        void setJValue(JNIEnv*, int index, jshort value) { values_[index].s = value; }
        void setJValue(JNIEnv*, int index, jint value) { values_[index].i = value; }
        void setJValue(JNIEnv*, int index, jlong value) { values_[index].j = value; }
        void setJValue(JNIEnv*, int index, jfloat value) { values_[index].f = value; }
        void setJValue(JNIEnv*, int index, jdouble value) { values_[index].d = value; }

        // explicitly handle nullptr
        void setJValue(JNIEnv*, int index, std::nullptr_t) { values_[index].l = nullptr; }

        // object types
        void setJValue(JNIEnv*, int index, jobject value) { values_[index].l = value; }

        // Handle other JNI reference types
        template <typename T>
        typename std::enable_if<std::is_convertible<T, jobject>::value, void>::type
        setJValue(JNIEnv*, int index, T value) { values_[index].l = value; }

        // Handle C++ string conversion to Java string
        void setJValue(JNIEnv* env, int index, const std::string& value) {
            jstring jstr = StringToJString(env, value);
            values_[index].l = jstr;
        }

        void setJValue(JNIEnv* env, int index, const char* value) {
            if (value == nullptr) {
                values_[index].l = nullptr;
            } else {
                jstring jstr = env->NewStringUTF(value);
                values_[index].l = jstr;
            }
        }
    };

    template <typename RetType, typename... Args>
    RetType CallMethod(JNIEnv* env, jobject obj, const char* methodName, const char* signature, Args... args) {
        jclass cls = env->GetObjectClass(obj);
        ScopedLocalRef<jclass> clsRef(env, cls);

        jmethodID mid = GetMethodID(env, cls, methodName, signature);

        if constexpr (sizeof...(Args) == 0) {
            // Handle the no-arguments case
            if constexpr (std::is_same_v<RetType, void>) {
                env->CallVoidMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return;
            }
            else if constexpr (std::is_same_v<RetType, jboolean>) {
                jboolean result = env->CallBooleanMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jbyte>) {
                jbyte result = env->CallByteMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jchar>) {
                jchar result = env->CallCharMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jshort>) {
                jshort result = env->CallShortMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jint>) {
                jint result = env->CallIntMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jlong>) {
                jlong result = env->CallLongMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jfloat>) {
                jfloat result = env->CallFloatMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jdouble>) {
                jdouble result = env->CallDoubleMethod(obj, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jstring> || std::is_convertible_v<RetType, jobject>) {
                RetType result = static_cast<RetType>(env->CallObjectMethod(obj, mid));
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else {
                static_assert(std::is_void_v<RetType> || std::is_convertible_v<RetType, jobject>,
                              "Unsupported return type for CallMethod");
                return {};
            }
        }
        else {
            // Handle the case with arguments
            ArgsToJValues<Args...> jvalues(env, args...);
            return JNITypeTraits<RetType>::CallMethod(env, obj, mid, jvalues.get());
        }
    }

    template <typename RetType, typename... Args>
    RetType CallStaticMethod(JNIEnv* env, const char* className, const char* methodName, const char* signature, Args... args) {
        jclass cls = FindClass(env, className);
        ScopedLocalRef<jclass> clsRef(env, cls);

        jmethodID mid = GetStaticMethodID(env, cls, methodName, signature);

        if constexpr (sizeof...(Args) == 0) {
            // Handle the no-arguments case
            if constexpr (std::is_same_v<RetType, void>) {
                env->CallStaticVoidMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return;
            }
            else if constexpr (std::is_same_v<RetType, jboolean>) {
                jboolean result = env->CallStaticBooleanMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jbyte>) {
                jbyte result = env->CallStaticByteMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jchar>) {
                jchar result = env->CallStaticCharMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jshort>) {
                jshort result = env->CallStaticShortMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jint>) {
                jint result = env->CallStaticIntMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jlong>) {
                jlong result = env->CallStaticLongMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jfloat>) {
                jfloat result = env->CallStaticFloatMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jdouble>) {
                jdouble result = env->CallStaticDoubleMethod(cls, mid);
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else if constexpr (std::is_same_v<RetType, jstring> || std::is_convertible_v<RetType, jobject>) {
                RetType result = static_cast<RetType>(env->CallStaticObjectMethod(cls, mid));
                JNI_CHECK_EXCEPTION(env);
                return result;
            }
            else {
                static_assert(std::is_void_v<RetType> || std::is_convertible_v<RetType, jobject>,
                              "Unsupported return type for CallStaticMethod");
                return {};
            }
        }
        else {
            // Handle the case with arguments
            ArgsToJValues<Args...> jvalues(env, args...);
            return JNITypeTraits<RetType>::CallStaticMethod(env, cls, mid, jvalues.get());
        }
    }

    template<typename... Args>
    jobject NewObject(JNIEnv* env, const char* className, const char* constructorSignature, Args... args) {
        jclass cls = FindClass(env, className);
        ScopedLocalRef<jclass> clsRef(env, cls);

        jmethodID constructor = GetMethodID(env, cls, "<init>", constructorSignature);

        ArgsToJValues<Args...> jvalues(env, args...);
        jobject obj = env->NewObjectA(cls, constructor, jvalues.get());
        JNI_CHECK_EXCEPTION(env);

        return obj;
    }

    template <typename T>
    T GetField(JNIEnv* env, jobject obj, const char* fieldName, const char* signature = nullptr) {
        jclass cls = env->GetObjectClass(obj);
        ScopedLocalRef<jclass> clsRef(env, cls);

        const char* fieldSig = signature ? signature : JNITypeTraits<T>::signature;
        jfieldID fid = GetFieldID(env, cls, fieldName, fieldSig);

        if (signature) {
            return static_cast<T>(JNITypeTraits<jobject>::GetField(env, obj, fid));
        } else {
            return JNITypeTraits<T>::GetField(env, obj, fid);
        }
    }

    template <typename T>
    T GetStaticField(JNIEnv* env, const char* className, const char* fieldName, const char* signature = nullptr) {
        jclass cls = FindClass(env, className);
        ScopedLocalRef<jclass> clsRef(env, cls);

        const char* fieldSig = signature ? signature : JNITypeTraits<T>::signature;
        jfieldID fid = GetStaticFieldID(env, cls, fieldName, fieldSig);

        if (signature) {
            return static_cast<T>(JNITypeTraits<jobject>::GetStaticField(env, cls, fid));
        } else {
            return JNITypeTraits<T>::GetStaticField(env, cls, fid);
        }
    }
} // namespace jni
