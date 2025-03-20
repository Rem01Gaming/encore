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
 * Note               : You can input inexact name with regex as this function utilize pgrep.
 ***********************************************************************************/
pid_t pidof(const char* name) {
    /*
     * It will execute something like this
     * /system/bin/toybox pgrep -o -f net.kdt.pojavlaunch
     *
     * You maybe asking, why we fetch PID like this?
     * Sometimes games like pojav launcher will have ridiculous process name such as
     * 'net.kdt.pojavlaunch:launcher', which doesn't match the game package name.
     */

    char* pid = execute_direct("/system/bin/toybox", "pgrep", "-o", "-f", name, NULL);

    if (pid) [[clang::likely]] {
        return atoi(pid);
    }

    return -1;
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
