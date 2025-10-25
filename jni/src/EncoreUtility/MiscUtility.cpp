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

#include <ShellUtility.hpp>

void set_do_not_disturb(bool do_not_disturb) {
    if (do_not_disturb) {
        system("cmd notification set_dnd priority");
    } else {
        system("cmd notification set_dnd off");
    }
}

void notify(const char *message) {
    pid_t pid = fork();

    if (pid == 0) {
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
            LOGE_TAG("Notify", "Push notification failed with status: {}", WEXITSTATUS(status));
        }
    } else {
        LOGE_TAG("Notify", "fork failed: {}", strerror(errno));
    }
}

void is_kanged(void) {
    if (systemv("grep -q '^name=Encore Tweaks$' %s", MODULE_PROP) != 0) [[unlikely]] {
        goto doorprize;
    }

    if (systemv("grep -q '^author=Rem01Gaming$' %s", MODULE_PROP) != 0) [[unlikely]] {
        goto doorprize;
    }

    return;

doorprize:
    LOGC("Module modified by 3rd party, exiting");
    notify("Trying to rename me?");
    exit(EXIT_FAILURE);
}
