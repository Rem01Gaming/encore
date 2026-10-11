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

#include "EncoreUtility.hpp"

#include <regex>

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

        const char *args[] = {"cmd", "notification", "set_dnd", do_not_disturb ? "priority" : "off", NULL};

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

        const char *args[] = {"cmd", "notification", "post", "-t", NOTIFY_TITLE, LOG_TAG, message, NULL};

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

// Runs a command with stdout/stderr silenced, returns its exit code
// (or -1 when the fork or the command itself fails).
static int run_silent(const char *path, char *const argv[]) {
    pid_t pid = fork();

    if (pid == 0) {
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }

        execvp(path, argv);
        _exit(127);
    }

    if (pid < 0) {
        LOGE("fork failed: {}", strerror(errno));
        return -1;
    }

    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

void toast(const char *message) {
    // Only proceed when the helper app is installed
    char *const check_args[] = {
        (char *)"cmd", (char *)"package", (char *)"path", (char *)"bellavita.toast", nullptr,
    };

    if (run_silent("/system/bin/cmd", check_args) != 0) {
        LOGD("Toast helper app is not installed, skipping toast");
        return;
    }

    char *const args[] = {
        (char *)"cmd",     (char *)"activity",     (char *)"start",
        (char *)"-a",      (char *)"android.intent.action.MAIN",
        (char *)"-e",      (char *)"toasttext",    (char *)message,
        (char *)"-n",      (char *)"bellavita.toast/.MainActivity",
        nullptr,
    };

    int rc = run_silent("/system/bin/cmd", args);
    if (rc != 0) [[unlikely]] {
        LOGE("Failed to show toast with status: {}", rc);
    }
}

ResumedState parse_resumed_activities(FILE *stream, const std::string &package) {
    // e.g. "topResumedActivity=ActivityRecord{1a2b3c u0 com.example.game/.MainActivity t42}"
    static const std::regex record_re(" u[0-9]+ ([A-Za-z0-9_.]+)/");

    bool seen_other_app = false;
    char *line = nullptr;
    size_t cap = 0;

    while (getline(&line, &cap, stream) != -1) {
        if (!strstr(line, "ResumedActivity")) continue;

        std::cmatch match;
        if (!std::regex_search(line, match, record_re)) continue;

        const std::string pkg = match[1].str();
        if (pkg == package) {
            free(line);
            return ResumedState::Resumed;
        }

        if (pkg != "com.android.systemui" && pkg != "android") {
            seen_other_app = true;
        }
    }

    free(line);
    return seen_other_app ? ResumedState::NotResumed : ResumedState::Unknown;
}

ResumedState query_package_resumed(const std::string &package) {
    if (package.empty()) return ResumedState::Unknown;

    FILE *pipe = popen("dumpsys activity activities 2>/dev/null", "r");
    if (!pipe) {
        LOGE("popen(dumpsys) failed: {}", strerror(errno));
        return ResumedState::Unknown;
    }

    ResumedState state = parse_resumed_activities(pipe, package);

    // A dumpsys that died halfway gives a partial dump, don't trust it
    if (pclose(pipe) != 0) return ResumedState::Unknown;
    return state;
}
