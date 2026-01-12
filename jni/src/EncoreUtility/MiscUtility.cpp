/*
 * Copyright (C) 2024-2025 Rem01Gaming
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

#include "EncoreUtility.hpp"

#include <ModuleProperty.hpp>
#include <ShellUtility.hpp>

void set_do_not_disturb(bool do_not_disturb) {
    pid_t pid = fork();

    if (pid == 0) {
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }

        const char *args[] = {
            "cmd", "notification", "set_dnd", do_not_disturb ? "priority" : "off", NULL};

        execvp("/system/bin/cmd", (char *const *)args);
        _exit(127);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) [[unlikely]] {
            LOGE("Failed to set DND mode with status: {}", WEXITSTATUS(status));
        }
    } else {
        LOGE("fork failed: {}", strerror(errno));
    }
}

void notify(const char *message) {
    pid_t pid = fork();

    if (pid == 0) {
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }

        if (setgid(2000) != 0 || setuid(2000) != 0) {
            _exit(126);
        }

        const char *args[] = {"cmd",        "notification", "post",  "-t",
                              NOTIFY_TITLE, LOG_TAG,        message, NULL};

        execvp("/system/bin/cmd", (char *const *)args);
        _exit(127);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) [[unlikely]] {
            LOGE("Push notification failed with status: {}", WEXITSTATUS(status));
        }
    } else {
        LOGE("fork failed: {}", strerror(errno));
    }
}

void is_kanged(void) {
    std::vector<ModuleProperties> module_properties;

    try {
        ModuleProperty::Get(MODULE_PROP, module_properties);

        for (const auto &property : module_properties) {
            if (property.key == "name" && property.value != "Encore Tweaks") {
                goto doorprize;
            }

            if (property.key == "author" && property.value != "Rem01Gaming") {
                goto doorprize;
            }
        }
    } catch (const std::exception &e) {
        LOGE_TAG("ModuleProperty", "{}", e.what());
    }

    return;

doorprize:
    LOGC("Module modified by 3rd party, exiting");
    notify("Trying to rename me?");
    exit(EXIT_FAILURE);
}
