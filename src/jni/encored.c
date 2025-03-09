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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define LOCK_FILE "/data/encore/encore.lock"
#define LOG_FILE "/dev/encore_log"
#define GAMELIST "/dev/encore_gamelist"
#define MODULE_PROP "/data/adb/modules/encore/module.prop"
#define MODULE_UPDATE "/data/adb/modules/encore/update"
#define GAME_STRESS "com.mobile.legends:UnityKillsMe"
#define MAX_COMMAND_LENGTH 600
#define MAX_OUTPUT_LENGTH 256

#define MY_PATH                                                                                                                    \
    "PATH=/system/bin:/system/xbin:/data/adb/ap/bin:/data/adb/ksu/bin:/data/adb/magisk:/debug_ramdisk:/sbin:/sbin/su:/su/bin:/su/" \
    "xbin:/data/data/com.termux/files/usr/bin"

#define IS_AWAKE(state) (strcmp(state, "Awake") == 0 || strcmp(state, "true") == 0)
#define IS_LOW_POWER(state) (strcmp(state, "true") == 0 || strcmp(state, "1") == 0)

typedef enum : char {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;

typedef enum : char {
    PERFCOMMON = 0,
    PERFORMANCE_PROFILE = 1,
    NORMAL_PROFILE = 2,
    POWERSAVE_PROFILE = 3
} ProfileMode;

typedef enum : char {
    MLBB_NOT_RUNNING = 0,
    MLBB_RUN_BG = 1,
    MLBB_RUNNING = 2
} MLBBState;

/***********************************************************************************
 * Function Name      : create_lock_file
 * Inputs             : None
 * Outputs            : None
 * Returns            : char - 0 if lock file created successfully
 *                            -1 if another instance running
 * Description        : Create lock file and check if there's any another instance of
 *                      this daemon running.
 ***********************************************************************************/
[[nodiscard]] char create_lock_file(void) {
    int fd = open(LOCK_FILE, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
        close(fd);
        return -1;
    }

    return 0;
}

/***********************************************************************************
 * Function Name      : ksu_grant_root
 * Inputs             : None
 * Outputs            : None
 * Returns            : char - 0 if granted successfully
 *                            -1 if request denied or error
 * Description        : Request SU permission from KernelSU via prctl.
 ***********************************************************************************/
[[nodiscard]] static inline char ksu_grant_root(void) {
    uint32_t result = 0;
    prctl(0xdeadbeef, 0, 0, 0, &result);

    if (result == 0xdeadbeef)
        return 0;

    return -1;
}

/***********************************************************************************
 * Function Name      : trim_newline
 * Inputs             : str (char *) - string to trim newline from
 * Outputs            : str (char *) - string without newline
 * Returns            : char * - pointer to the modified string
 * Description        : Trims a newline character at the end of a string if
 *                      present.
 ***********************************************************************************/
[[gnu::always_inline]] static inline char* trim_newline(char* string) {
    if (string == NULL)
        return NULL;

    char* end;
    if ((end = strchr(string, '\n')) != NULL)
        *end = '\0';

    return string;
}

/***********************************************************************************
 * Function Name      : timern
 * Inputs             : None
 * Outputs            : Formatted timestamp
 * Returns            : char * - pointer to a statically allocated string
 *                      with the formatted time.
 * Description        : Generates a timestamp with the format
 *                      [Day Mon DD HH:MM:SS YYYY].
 ***********************************************************************************/
static inline char* timern(void) {
    static char timestamp[64];
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);

    if (tm == NULL) {
        strcpy(timestamp, "[TimeError]");
        return timestamp;
    }

    size_t ret = strftime(timestamp, sizeof(timestamp), "%c", tm);
    if (ret == 0) {
        strcpy(timestamp, "[TimeFormatError]");
    }

    return timestamp;
}

/***********************************************************************************
 * Function Name      : write2file
 * Inputs             : file_path (const char *) - path to the file
 *                      content (const char *) - content to write
 *                      mode (const int) - 1 for append and 0 for write
 * Outputs            : None
 * Returns            : char - 0 if write successful
 *                            -1 if file does not exist or inaccessible
 * Description        : Write the provided content to the specified file.
 ***********************************************************************************/
static inline char write2file(const char* file_path, const char* content, const int mode) {
    if (access(file_path, F_OK) == -1)
        return -1;

    const char* write_mode;
    switch (mode) {
    case 0:
        write_mode = "w";
        break;
    case 1:
        write_mode = "a";
        break;
    default:
        write_mode = "w";
    }

    FILE* file = fopen(file_path, write_mode);
    if (file == NULL)
        return -1;

    fprintf(file, "%s\n", content);
    fclose(file);
    return 0;
}

/***********************************************************************************
 * Function Name      : log_encore
 * Inputs             : level - Log level
 *                      message (const char *) - message to log
 *                      variadic arguments - additional arguments for message
 * Outputs            : None
 * Returns            : None
 * Description        : print and logs a formatted message with a timestamp
 *                      to a log file.
 ***********************************************************************************/
void log_encore(LogLevel level, const char* message, ...) {
    const char* level_str[] = {"DEBUG", "INFO ", "WARN ", "ERROR", "FATAL"};

    char* timestamp = timern();
    char logMesg[MAX_OUTPUT_LENGTH];
    va_list args;
    va_start(args, message);
    vsnprintf(logMesg, sizeof(logMesg), message, args);
    va_end(args);

    char logEncore[MAX_OUTPUT_LENGTH];
    snprintf(logEncore, sizeof(logEncore), "[%s] [%s] %s", timestamp, level_str[level], logMesg);
    write2file(LOG_FILE, logEncore, 1);
}

/***********************************************************************************
 * Function Name      : sighandler
 * Inputs             : int signal - exit signal
 * Outputs            : None
 * Returns            : None
 * Description        : Handle exit signal.
 ***********************************************************************************/
[[noreturn]] void sighandler(const int signal) {
    switch (signal) {
    case SIGTERM:
        log_encore(LOG_INFO, "Received SIGTERM, exiting.");
        break;
    case SIGINT:
        log_encore(LOG_INFO, "Received SIGINT, exiting.");
        break;
    }

    // Exit gracefully
    _exit(EXIT_SUCCESS);
}

/***********************************************************************************
 * Function Name      : execute_command
 * Inputs             : command (const char *) - shell command to execute
 * Outputs            : None
 * Returns            : char * - pointer to the dynamically allocated output of
 *                      the command execution
 *                      variadic arguments - Additional arguments for command
 * Description        : Executes a shell command and captures its output.
 ***********************************************************************************/
char* execute_command(const char* format, ...) {
    if (format == NULL)
        return NULL;

    char command[MAX_COMMAND_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(command, sizeof(command), format, args);
    va_end(args);

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        log_encore(LOG_ERROR, "pipe failed in execute_command()");
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
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
 * Outputs            : None
 * Returns            : char * - Pointer to the dynamically allocated output of the command
 * Description        : Executes a binary directly with specified arguments and captures output.
 * Note               : Caller is responsible for freeing the returned string.
 ***********************************************************************************/
char* execute_direct(const char* path, const char* arg0, ...) {
    if (path == NULL || arg0 == NULL)
        return NULL;

    const char* argv[16]; // Supports up to 15 arguments + NULL
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
    if (pipe(pipefd) == -1) {
        log_encore(LOG_ERROR, "pipe failed in execute_direct()");
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
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
 * Outputs            : None
 * Returns            : int - 0 if execution success
 *                           -1 if execution failed
 *                            * otherwise if command returns an error
 * Description        : Executes a shell command just like system() with additional format.
 ***********************************************************************************/
[[nodiscard("Shell command can be faulty and you should check it's returns!")]]
int systemv(const char* format, ...) {
    if (format == NULL)
        return -1;

    char command[MAX_COMMAND_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(command, sizeof(command), format, args);
    va_end(args);

    pid_t pid = fork();
    if (pid == -1) {
        log_encore(LOG_ERROR, "fork failed in systemv()");
        return -1;
    }

    if (pid == 0) {
        char* env[] = {MY_PATH, NULL};
        execle("/system/bin/sh", "sh", "-c", command, NULL, env);

        // If exec fails
        _exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) == -1)
        return -1;

    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    return -1;
}

/***********************************************************************************
 * Function Name      : notify
 * Inputs             : message (char *) - Message to display
 * Outputs            : None
 * Returns            : None
 * Description        : Push a notification.
 ***********************************************************************************/
static inline void notify(const char* message) {
    int exit = systemv("su -lp 2000 -c \"/system/bin/cmd notification post -t 'Encore Tweaks' 'encore' '%s'\" >/dev/null", message);

    if (exit != 0)
        log_encore(LOG_ERROR, "Unable to post push notification, message: %s", message);
}

/***********************************************************************************
 * Function Name      : set_priority
 * Inputs             : pid (const char *) - PID as a string
 * Outputs            : None
 * Returns            : None
 * Description        : Sets the CPU nice priority and I/O priority of a given
 *                      process.
 ***********************************************************************************/
static inline void set_priority(const char* pid) {
    if (pid == NULL) [[clang::unlikely]] {
        log_encore(LOG_ERROR, "set_priority() called with null PID!");
        return;
    }

    pid_t process_id = atoi(pid);
    log_encore(LOG_DEBUG, "Applying priority settings for PID %s", pid);

    if (setpriority(PRIO_PROCESS, process_id, -20) == -1)
        log_encore(LOG_ERROR, "Unable to set nice priority for %s", pid);

    if (syscall(SYS_ioprio_set, 1, process_id, (1 << 13) | 0) == -1)
        log_encore(LOG_ERROR, "Unable to set IO priority for %s", pid);
}

/***********************************************************************************
 * Function Name      : is_kanged
 * Inputs             : None
 * Outputs            : None
 * Returns            : None
 * Description        : Checks if the module renamed/modified by 3rd party.
 ***********************************************************************************/
static inline void is_kanged(void) {
    if (systemv("grep -q 'author=Rem01Gaming' %s", MODULE_PROP) != 0) {
        log_encore(LOG_FATAL, "Module modified by 3rd party, exiting.");
        notify("Trying to rename me?");
        exit(EXIT_FAILURE);
    }
}

/***********************************************************************************
 * Function Name      : run_profiler
 * Inputs             : int - 0 for perfcommon
 *                            1 for performance
 *                            2 for normal
 *                            3 for powersave
 * Outputs            : None
 * Returns            : None
 * Description        : Switch to specified performance profile.
 ***********************************************************************************/
static inline void run_profiler(const int profile) {
    is_kanged();
    char profile_str[16];
    snprintf(profile_str, sizeof(profile_str), "%d", profile);
    write2file("/dev/encore_mode", profile_str, 0);
    (void)systemv("encore_profiler %d", profile);
}

/***********************************************************************************
 * Function Name      : get_gamestart
 * Inputs             : None
 * Outputs            : None
 * Returns            : char* (dynamically allocated string with the game package name)
 * Description        : Searches for the currently visible application that matches
 *                      any package name listed in /data/encore/gamelist.txt.
 *                      This helps identify if a specific game is running in the foreground.
 *                      Uses dumpsys to retrieve visible apps and filters by packages
 *                      listed in Gamelist.
 * Note               : Caller is responsible for freeing the returned string.
 ***********************************************************************************/
static inline char* get_gamestart(void) {
    return execute_command("dumpsys window visible-apps | grep 'package=.* ' | grep -Eo -f %s", GAMELIST);
}

/***********************************************************************************
 * Function Name      : get_screenstate
 * Inputs             : None
 * Outputs            : None
 * Returns            : char* ("Awake" or "Asleep" based on screen state)
 * Description        : Retrieves the current screen wakefulness state (Awake or Asleep).
 *                      If not available, falls back to checking if the device is awake
 *                      through an alternative dumpsys window command.
 * Note               : Caller is responsible for freeing the returned string.
 ***********************************************************************************/
static inline char* get_screenstate(void) {
    char* state = execute_command("dumpsys power | grep -Eo 'mWakefulness=Awake|mWakefulness=Asleep' "
                                  "| awk -F'=' '{print $2}'");
    if (state == NULL) {
        state = execute_command("dumpsys window displays | grep -Eo 'mAwake=true|mAwake=false' | "
                                "awk -F'=' '{print $2}'");
    }
    return state;
}

/***********************************************************************************
 * Function Name      : get_low_power_state
 * Inputs             : None
 * Outputs            : None
 * Returns            : char* ("true" or "1" if Battery Saver is enabled, "false" otherwise)
 * Description        : Checks if the device's Battery Saver mode is enabled by using
 *                      dumpsys power and filtering for the battery saver status.
 *                      Useful for determining low-power states.
 * Note               : Caller is responsible for freeing the returned string.
 ***********************************************************************************/
static inline char* get_low_power_state(void) {
    char* low_power = execute_direct("/system/bin/settings", "settings", "get", "global", "low_power", NULL);
    if (low_power == NULL) {
        low_power = execute_command("dumpsys power | grep -Eo "
                                    "'mSettingBatterySaverEnabled=true|mSettingBatterySaverEnabled=false' | "
                                    "awk -F'=' '{print $2}'");
    }
    return low_power;
}

/***********************************************************************************
 * Function Name      : pidof
 * Inputs             : name (char *) - Name of process
 * Outputs            : pid (char *) - PID of process
 * Returns            : PID of process
 * Description        : Fetch PID of a program
 * Note               : Caller is responsible for freeing the returned string.
 ***********************************************************************************/
static inline char* pidof(const char* name) {
    /*
     * It will execute something like this
     * /system/bin/toybox pgrep -o -f net.kdt.pojavlaunch
     *
     * You maybe asking, why we fetch PID like this?
     * Sometimes games like pojav launcher will have ridiculous process name such as
     * 'net.kdt.pojavlaunch:launcher', which doesn't match the game package name.
     */

    return execute_direct("/system/bin/toybox", "pgrep", "-o", "-f", name, NULL);
}

/***********************************************************************************
 * Function Name      : handle_mlbb
 * Inputs             : const char *gamestart - Game package name
 * Outputs            : None
 * Returns            : char - 2 if MLBB is running in foreground
 *                             1 if MLBB is running in background
 *                             0 if gamestart is not MLBB
 * Description        : Checks if Mobile Legends: Bang Bang IS actually running
 *                      on foreground, not in the background.
 ***********************************************************************************/
static inline char handle_mlbb(const char* gamestart) {
    static pid_t cached_pid = -1;

    // Is Gamestart MLBB?
    if (strcmp(gamestart, "com.mobile.legends") != 0) {
        cached_pid = -1;
        return 0;
    }

    // Check if cached PID is still valid
    if (cached_pid != -1) {
        // Does MLBB is still running?
        if (kill(cached_pid, 0) == 0)
            return 2;

        // Process died, reset cache
        cached_pid = -1;
    }

    // Fetch new PID if cache is invalid
    char* mlbb_pid = pidof(GAME_STRESS);
    if (mlbb_pid != NULL) {
        cached_pid = atoi(mlbb_pid);
        free(mlbb_pid);
        return 2;
    }

    // MLBB is in background
    return 1;
}

int main(void) {
    // Handle case when not running on root
    // Try grant KSU ROOT via prctl
    if (getuid() != 0 && ksu_grant_root() != 0) {
        fprintf(stderr, "\033[31mERROR:\033[0m Please run this daemon as root\n");
        exit(EXIT_FAILURE);
    }

    // Make sure only one instance is running
    if (create_lock_file() != 0) {
        fprintf(stderr, "\033[31mERROR:\033[0m Another instance of Encore Daemon is already running!\n");
        exit(EXIT_FAILURE);
    }

    // Handle case when module modified by 3rd party
    is_kanged();

    // Handle missing Gamelist
    if (access(GAMELIST, F_OK) != 0) {
        fprintf(stderr, "\033[31mFATAL ERROR:\033[0m Unable to access Gamelist, either has been removed or moved.\n");
        log_encore(LOG_FATAL, "Critical file not found (%s)", GAMELIST);
        exit(EXIT_FAILURE);
    }

    // Daemonize service
    if (daemon(0, 0)) {
        log_encore(LOG_FATAL, "Unable to daemonize service");
        exit(EXIT_FAILURE);
    }

    // Register signal handlers
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    // Initialize variables
    char *gamestart = NULL, *screenstate = NULL, *low_power = NULL, *pid = NULL, screenstate_fail = 0;
    MLBBState mlbb_is_running = MLBB_NOT_RUNNING;
    ProfileMode cur_mode = PERFCOMMON;

    log_encore(LOG_INFO, "Daemon started as PID %d", getpid());
    run_profiler(PERFCOMMON); // exec perfcommon

    while (1) {
        if (low_power) {
            free(low_power);
            low_power = NULL;
        }

        sleep(15);

        if (!gamestart) {
            // Only fetch gamestart and low_power state when user not in-game
            // prevent overhead from dumpsys commands.
            gamestart = get_gamestart();
            low_power = get_low_power_state();
        } else if (!pid || kill(atoi(pid), 0) == -1) {
            free(pid);
            pid = NULL;
            free(gamestart);
            gamestart = get_gamestart();
            low_power = get_low_power_state();

            if (gamestart) {
                pid = pidof(gamestart);
                set_priority(pid);
            }
        }

        if (gamestart)
            mlbb_is_running = handle_mlbb(gamestart);

        // If screenstate_fail threshold eq to 6, skip this routine entirely
        if (screenstate_fail != 6) {
            // Fetch screenstate
            free(screenstate);
            screenstate = get_screenstate();

            // Handle in case screenstate is empty
            if (screenstate == NULL) [[clang::unlikely]] {
                screenstate_fail++;
                log_encore(LOG_ERROR, "Unable to get current screenstate");
                log_encore(LOG_DEBUG, "screenstate fail count: %s", screenstate_fail);

                // Set default state after too many failures
                if (screenstate_fail == 6) {
                    log_encore(LOG_WARN, "Too much error, assume screenstate was awake anytime from now!");
                    screenstate = "Awake";
                }

                continue;
            } else {
                // Reset failure counter if screenstate is valid
                screenstate_fail = 0;
            }
        }

        // Handle case when module gets updated
        if (access(MODULE_UPDATE, F_OK) == 0) {
            log_encore(LOG_INFO, "Module update detected, exiting.");
            notify("Please reboot your device to complete module update.");
            break;
        }

        if (gamestart && IS_AWAKE(screenstate) && mlbb_is_running != MLBB_RUN_BG) {
            // Bail out if we already on performance profile
            if (cur_mode == PERFORMANCE_PROFILE)
                continue;

            // Get PID and check if the game is "real" running program
            // Handle weird behavior of MLBB
            pid = (mlbb_is_running == MLBB_RUNNING) ? pidof(GAME_STRESS) : pidof(gamestart);
            if (pid == NULL) {
                log_encore(LOG_ERROR, "Unable to fetch PID of %s", gamestart);
                continue;
            }

            cur_mode = PERFORMANCE_PROFILE;
            log_encore(LOG_INFO, "Applying performance profile for %s", gamestart);
            run_profiler(PERFORMANCE_PROFILE);
            set_priority(pid);
        } else if (low_power && IS_LOW_POWER(low_power)) {
            // Bail out if we already on powersave profile
            if (cur_mode == POWERSAVE_PROFILE)
                continue;

            cur_mode = POWERSAVE_PROFILE;
            log_encore(LOG_INFO, "Applying powersave profile");
            run_profiler(POWERSAVE_PROFILE);
        } else {
            // Bail out if we already on normal profile
            if (cur_mode == NORMAL_PROFILE)
                continue;

            cur_mode = NORMAL_PROFILE;
            log_encore(LOG_INFO, "Applying normal profile");
            run_profiler(NORMAL_PROFILE);
        }
    }

    return 0;
}
