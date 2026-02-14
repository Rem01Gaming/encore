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

#pragma once

#include <cstdio>
#include <memory>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

struct PipeResult {
    FILE* stream;
    pid_t pid;

    PipeResult(FILE* s, pid_t p) : stream(s), pid(p) {}

    // Helper to close and reap automatically
    void close() {
        if (stream) {
            fclose(stream);
            stream = nullptr;
        }
        if (pid > 0) {
            waitpid(pid, nullptr, 0);
            pid = -1;
        }
    }

    ~PipeResult() { close(); }
    
    // Disable copying
    PipeResult(const PipeResult&) = delete;
    PipeResult& operator=(const PipeResult&) = delete;
    
    // Allow moving
    PipeResult(PipeResult&& other) noexcept : stream(other.stream), pid(other.pid) {
        other.stream = nullptr;
        other.pid = -1;
    }
};

/**
 * @brief Executes a command directly and captures its standard output.
 *
 * Forks a child process and uses `execvp` to run the specified command. This approach 
 * is safer than `popen` as it avoids shell interpretation and potential injection.
 * 
 * @param args A vector where args[0] is the command and subsequent elements are arguments.
 * @return A PipeResult object. Check `PipeResult.stream` for the file pointer.
 * @note The child process is automatically reaped (waitpid) when the result goes out of scope.
 */
inline PipeResult popen_direct(const std::vector<std::string> &args) {
    int pipefd[2];
    if (pipe(pipefd) == -1) return PipeResult(nullptr, -1);
    pid_t pid = fork();
    if (pid == -1) {
        ::close(pipefd[0]);
        ::close(pipefd[1]);
        return PipeResult(nullptr, -1);
    }

    if (pid == 0) { // Child
        ::close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        ::close(pipefd[1]);

        std::vector<char *> cargs;
        for (const auto &arg : args) {
            cargs.push_back(const_cast<char *>(arg.c_str()));
        }
        cargs.push_back(nullptr);

        execvp(cargs[0], cargs.data());
        _exit(127);
    } 

    // Parent
    ::close(pipefd[1]);
    return PipeResult(fdopen(pipefd[0], "r"), pid);
}

/**
 * @brief Executes a shell command with formatted arguments.
 *
 * This function is a wrapper around the standard `system()` call, providing `printf`-like
 * formatting for constructing the command string.
 *
 * @param format A format string as you would use with `printf`.
 * @param ... Additional arguments to be formatted into the command string.
 * @return The return value of the `system()` call.
 */
inline int systemv(const char *format, ...) {
    char command[512];
    va_list args;
    va_start(args, format);
    vsnprintf(command, sizeof(command), format, args);
    va_end(args);
    return system(command);
}
