/*
 * Copyright (C) 2026 Rem01Gaming
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

#include <jni.h>
#include <cstdint>
#include <string>
#include <vector>

#include <logging.hpp>
#include <Zygisk.hpp>
#include <JniHelperZygisk.hpp>
#include <dex_loader.hpp>

#include <dex_blob.hpp>

static bool loadDexAndCall(JNIEnv *env, const char *className, const char *methodName,
                           const char *signature, bool passClassLoader) {
    std::vector<uint8_t> dexBytes(dex_bytes, dex_bytes + dex_len);

    try {
        jobject loaderLocal = dexloader::detail::CreateInMemoryClassLoader(env, dexBytes);
        jni::ScopedLocalRef<jobject> loaderRef(env, loaderLocal);

        jclass entryClsLocal = dexloader::detail::LoadClassWithClassLoader(
            env, loaderRef.get(), className);
        jni::ScopedLocalRef<jclass> entryClsRef(env, entryClsLocal);

        jmethodID method = jni::GetStaticMethodID(
            env, entryClsRef.get(), methodName, signature);

        if (passClassLoader) {
            env->CallStaticVoidMethod(entryClsRef.get(), method, loaderRef.get());
        } else {
            env->CallStaticVoidMethod(entryClsRef.get(), method);
        }

        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
            return false;
        }

        return true;
    } catch (const std::exception &e) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        LOGE("[EncoreTweaks] %s", e.what());
        return false;
    }
}

// =============================================================================
// Zygisk lifecycle
// =============================================================================

class EncoreTweaks : public zygisk::ModuleBase {
private:
    zygisk::Api *api = nullptr;
    JNIEnv *env = nullptr;
    bool isSystemUi = false;

public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void postServerSpecialize(const zygisk::ServerSpecializeArgs *args) override {
        (void) args;

        if (!loadDexAndCall(env, "com.rem01gaming.encore.BinderResolver", "start", "()V", false)) {
            LOGE("[EncoreTweaks] failed to start BinderResolver");
        }
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
        if (!args || !args->nice_name) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        std::string pkg = jni::JStringToString(env, args->nice_name);
        isSystemUi = (pkg == "com.android.systemui");

        if (!isSystemUi) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override {
        (void) args;

        if (!isSystemUi) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        if (!loadDexAndCall(env, "com.rem01gaming.encore.ToastBridge", "install",
                    "(Ljava/lang/ClassLoader;)V", true)) {
            LOGE("[EncoreTweaks] failed to install dex in SystemUI");
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
    }
};

REGISTER_ZYGISK_MODULE(EncoreTweaks)
