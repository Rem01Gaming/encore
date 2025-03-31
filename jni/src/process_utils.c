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
 * Function Name      : pidof
 * Inputs             : name (char *) - Name of process
 * Returns            : pid (pid_t) - PID of process
 * Description        : Fetch PID from a process name.
 * Note               : You can input inexact process name.
 ***********************************************************************************/
pid_t pidof(const char *name) {
    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) [[clang::unlikely]] {
        perror("opendir");
        return 0;
    }

    pid_t tracked_pid = 0;
    struct dirent *entry;

    while ((entry = readdir(proc_dir))) {
        if (entry->d_type != DT_DIR)
            continue;

        // Check if directory name is a valid PID
        bool is_pid = true;
        for (char *p = entry->d_name; *p; ++p) {
            if (!isdigit((unsigned char)*p)) {
                is_pid = false;
                break;
            }
        }

        if (!is_pid) [[clang::unlikely]] {
            continue;
        }

        // Read cmdline
        char path[256];
        snprintf(path, sizeof(path), "/proc/%s/cmdline", entry->d_name);
        FILE *fp = fopen(path, "r");

        if (!fp) [[clang::unlikely]] {
            continue;
        }

        char cmdline[4096];
        size_t len = fread(cmdline, 1, sizeof(cmdline) - 1, fp);
        fclose(fp);

        if (len == 0) [[clang::unlikely]] {
            continue;
        }

        // Replace null bytes with spaces
        for (size_t i = 0; i < len; ++i) {
            if (cmdline[i] == '\0')
                cmdline[i] = ' ';
        }
        cmdline[len] = '\0';

        // Check for substring match
        if (strstr(cmdline, name) != NULL) {
            char *end;
            long pid_val = strtol(entry->d_name, &end, 10);
            if (end == entry->d_name || *end != '\0' || pid_val <= 0)
                continue;

            if (tracked_pid == 0 || pid_val < tracked_pid)
                tracked_pid = (pid_t)pid_val;
        }
    }

    closedir(proc_dir);
    return tracked_pid;
}

/***********************************************************************************
 * Function Name      : set_priority
 * Inputs             : pid (pid_t) - PID to be boosted
 * Returns            : None
 * Description        : Sets the maximum CPU nice priority and I/O priority of a
 *                      given process.
 ***********************************************************************************/
void set_priority(const pid_t pid) {
    log_encore(LOG_DEBUG, "Applying priority settings for PID %d", pid);

    if (setpriority(PRIO_PROCESS, pid, -20) == -1)
        log_encore(LOG_ERROR, "Unable to set nice priority for %d", pid);

    if (syscall(SYS_ioprio_set, 1, pid, (1 << 13) | 0) == -1)
        log_encore(LOG_ERROR, "Unable to set IO priority for %d", pid);
}
