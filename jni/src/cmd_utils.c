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

#include <encore.h>

/***********************************************************************************
 * Function Name      : execute_command
 * Inputs             : command (const char *) - shell command to execute
 * Returns            : char * - Pointer to the dynamically allocated output of the command
 *                      variadic arguments - Additional arguments for command
 * Description        : Executes a shell command and captures its output.
 ***********************************************************************************/
char* execute_command(const char* format, ...) {
    char command[MAX_COMMAND_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(command, sizeof(command), format, args);
    va_end(args);

    int pipefd[2];
    if (pipe(pipefd) == -1) [[clang::unlikely]] {
        log_encore(LOG_ERROR, "pipe failed in execute_command()");
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) [[clang::unlikely]] {
        close(pipefd[0]);
        close(pipefd[1]);
        log_encore(LOG_ERROR, "fork failed in execute_command()");
        return NULL;
    }

    if (pid == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        char* env[] = {MY_PATH, NULL};
        execle("/system/bin/sh", "sh", "-c", command, NULL, env);
        _exit(127);
    }

    close(pipefd[1]);

    char output[MAX_OUTPUT_LENGTH] = {0};
    ssize_t total_read = 0;
    while (1) {
        ssize_t bytes = read(pipefd[0], output + total_read, sizeof(output) - total_read - 1);
        if (bytes <= 0)
            break;

        total_read += bytes;

        if (total_read >= (ssize_t)(sizeof(output) - 1))
            break;
    }
    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);
    if (WEXITSTATUS(status))
        return NULL;

    return strdup(trim_newline(output));
}

/***********************************************************************************
 * Function Name      : execute_direct
 * Inputs             : path (const char *) - Path to the executable
 *                      arg0 (const char *) - First argument (typically the program name)
 *                      variadic arguments - Additional arguments, must end with NULL
 * Returns            : char * - Pointer to the dynamically allocated output of the command
 * Description        : Executes a binary directly with specified arguments and captures output.
 * Note               : Caller is responsible for freeing the returned string.
 ***********************************************************************************/
char* execute_direct(const char* path, const char* arg0, ...) {
    // Supports up to 15 arguments + NULL
    const char* argv[16];
    int argc = 0;
    argv[argc++] = arg0;

    va_list args;
    va_start(args, arg0);
    const char* arg;
    while ((arg = va_arg(args, const char*)) && argc < 15) {
        argv[argc++] = arg;
    }
    argv[argc] = NULL;
    va_end(args);

    int pipefd[2];
    if (pipe(pipefd) == -1) [[clang::unlikely]] {
        log_encore(LOG_ERROR, "pipe failed in execute_direct()");
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) [[clang::unlikely]] {
        close(pipefd[0]);
        close(pipefd[1]);
        log_encore(LOG_ERROR, "fork failed in execute_direct()");
        return NULL;
    }

    if (pid == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        execv(path, (char* const*)argv);
        _exit(127);
    }

    close(pipefd[1]);

    char output[MAX_OUTPUT_LENGTH] = {0};
    ssize_t total_read = 0;
    while (1) {
        ssize_t bytes = read(pipefd[0], output + total_read, sizeof(output) - total_read - 1);
        if (bytes <= 0)
            break;

        total_read += bytes;

        if (total_read >= (ssize_t)(sizeof(output) - 1))
            break;
    }
    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);
    if (WEXITSTATUS(status))
        return NULL;

    return strdup(trim_newline(output));
}

/***********************************************************************************
 * Function Name      : systemv
 * Inputs             : format (const char *) - shell command to execute
 *                      variadic arguments - other arguments
 * Returns            : int - non zero are error, following system() returns.
 * Description        : Executes a shell command just like system() with additional format.
 ***********************************************************************************/
int systemv(const char* format, ...) {
    char command[MAX_COMMAND_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(command, sizeof(command), format, args);
    va_end(args);
    return system(command);
}
