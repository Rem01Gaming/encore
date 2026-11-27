#pragma once

#include <jni.h>

#include <string>
#include <utility>
#include <vector>

#include "dex_loader.hpp"
#include "JniHelperZygisk.hpp"

namespace dexloader {

    inline void EnsureJavaVM(JNIEnv* env) {
        if (env == nullptr) {
            return;
        }

        JavaVM* vm = nullptr;
        if (env->GetJavaVM(&vm) == JNI_OK && vm != nullptr) {
            jni::zygisk::VM::Init(vm);
        }
    }

/**
 * Keeps the DEX bytes, ClassLoader, and entry class alive.
 *
 * Useful if the loaded entry point must remain reachable after loading.
 */
    class PersistentInMemoryDexLoader {
    public:
        using Config = InMemoryDexLoader::Config;

        explicit PersistentInMemoryDexLoader(Config config)
                : config_(std::move(config)) {}

        PersistentInMemoryDexLoader(const PersistentInMemoryDexLoader&) = delete;
        PersistentInMemoryDexLoader& operator=(const PersistentInMemoryDexLoader&) = delete;

        PersistentInMemoryDexLoader(PersistentInMemoryDexLoader&&) = delete;
        PersistentInMemoryDexLoader& operator=(PersistentInMemoryDexLoader&&) = delete;

        ~PersistentInMemoryDexLoader() = default;

        bool Load(JNIEnv* env,
                  std::vector<uint8_t> dexBytes,
                  std::string* error) {
            try {
                EnsureJavaVM(env);

                Unload();

                if (env == nullptr) {
                    throw jni::JNIException("JNIEnv is null");
                }

                if (config_.entryClassName.empty()) {
                    throw jni::JNIException("entryClassName is empty");
                }

                dexBytes_ = std::move(dexBytes);

                jobject classLoaderLocal = detail::CreateInMemoryClassLoader(env, dexBytes_);
                classLoader_ = jni::zygisk::ScopedGlobalRef<jobject>(env, classLoaderLocal);

                jclass entryClassLocal = detail::LoadClassWithClassLoader(
                        env,
                        classLoader_.get(),
                        config_.entryClassName
                );
                entryClass_ = jni::zygisk::ScopedGlobalRef<jclass>(env, entryClassLocal);

                return true;
            } catch (const std::exception& e) {
                detail::ClearPendingException(env);
                if (error != nullptr) {
                    *error = e.what();
                }
                Unload();
                return false;
            } catch (...) {
                detail::ClearPendingException(env);
                if (error != nullptr) {
                    *error = "Unknown persistent DEX loader error";
                }
                Unload();
                return false;
            }
        }

        bool CallInit(JNIEnv* env,
                      const std::string& jsonConfig,
                      bool flag1,
                      bool flag2,
                      bool flag3,
                      std::string* error) const {
            try {
                if (env == nullptr) {
                    throw jni::JNIException("JNIEnv is null");
                }

                if (entryClass_.get() == nullptr) {
                    throw jni::JNIException("DEX entry class is not loaded");
                }

                jmethodID entryInit = jni::GetStaticMethodID(
                        env,
                        entryClass_.get(),
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
                        entryClass_.get(),
                        entryInit,
                        jsonRef.get(),
                        static_cast<jboolean>(flag1 ? JNI_TRUE : JNI_FALSE),
                        static_cast<jboolean>(flag2 ? JNI_TRUE : JNI_FALSE),
                        static_cast<jboolean>(flag3 ? JNI_TRUE : JNI_FALSE)
                );
                JNI_CHECK_EXCEPTION(env);

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
                    *error = "Unknown persistent DEX init error";
                }
                return false;
            }
        }

        bool IsLoaded() const {
            return entryClass_.get() != nullptr;
        }

        void Unload() noexcept {
            classLoader_ = {};
            entryClass_ = {};
            dexBytes_.clear();
        }

    private:
        Config config_;

        // Kept alive because InMemoryDexClassLoader may need the buffer to remain valid.
        std::vector<uint8_t> dexBytes_;

        jni::zygisk::ScopedGlobalRef<jobject> classLoader_;
        jni::zygisk::ScopedGlobalRef<jclass> entryClass_;
    };

} // namespace dexloader
