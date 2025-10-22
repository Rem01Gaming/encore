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

#pragma once

#include <cstdio>
#include <memory>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

/**
 * @brief Opens a process by creating a pipe, forking, and invoking execvp.
 *
 * This function is a replacement for `popen` that directly takes a vector of strings
 * as arguments, avoiding the need for shell interpretation. This is safer as it
 * prevents shell injection vulnerabilities. The function creates a pipe, forks the
 * process, and in the child process, executes the command specified by `args`.
 * The standard output of the child process is redirected to the pipe. The parent
 * process receives a `FILE*` stream to read from the child's standard output.
 *
 * @param args A vector of strings where the first element is the command to execute
 *             and subsequent elements are its arguments.
 * @return A `std::unique_ptr<FILE, decltype(&fclose)>` that wraps the file stream
 *         for reading the child's stdout. The file is automatically closed when the
 *         unique_ptr goes out of scope. Returns a null-holding unique_ptr on failure.
 */
inline std::unique_ptr<FILE, decltype(&fclose)> popen_direct(const std::vector<std::string> &args) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return {nullptr, fclose};
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        return {nullptr, fclose};
    }

    if (pid == 0) { // Child
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        std::vector<char *> cargs;
        for (const auto &arg : args) {
            cargs.push_back(const_cast<char *>(arg.c_str()));
        }
        cargs.push_back(nullptr);

        execvp(cargs[0], cargs.data());
        _exit(127);
    } else { // Parent
        close(pipefd[1]);
        return {fdopen(pipefd[0], "r"), fclose};
    }
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
inline int systemv(const char* format, ...) {
    char command[512];
    va_list args;
    va_start(args, format);
    vsnprintf(command, sizeof(command), format, args);
    va_end(args);
    return system(command);
}
