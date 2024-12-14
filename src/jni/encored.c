#include "module_drm.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define LOG_FILE "/data/encore/encore_log"
#define MODULE_PROP "/data/adb/modules/encore/module.prop"
#define MAX_COMMAND_LENGTH 1024
#define MAX_OUTPUT_LENGTH 150
#define MAX_PATH_LENGTH 256

/***********************************************************************************
 * Function Name      : trim_newline
 * Inputs             : str (char *) - string to trim newline from
 * Outputs            : str (char *) - string without newline
 * Returns            : char * - pointer to the modified string
 * Description        : Trims a newline character at the end of a string if
 *                      present.
 ***********************************************************************************/
static inline char* trim_newline(char* string) {
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
 * Returns            : char * - pointer to a dynamically allocated string
 *                      with the formatted time.
 * Description        : Generates a timestamp with the format
 *                      [Day Mon DD HH:MM:SS YYYY].
 ***********************************************************************************/
char* timern(void) {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    char* timestamp = malloc(64 * sizeof(char));
    if (timestamp == NULL) {
        printf("error: memory allocation failed in timern()\n");
        return NULL;
    }
    size_t ret = strftime(timestamp, 64, "%c", tm);
    if (ret == 0) {
        printf("error: strftime failed in timern()\n");
        free(timestamp);
        return NULL;
    }
    return timestamp;
}

/***********************************************************************************
 * Function Name      : append2file
 * Inputs             : file_path (const char *) - path to the file
 *                      content (const char *) - content to append
 * Outputs            : None
 * Returns            : int - 0 if write successful
 *                           -1 if file does not exist or inaccessible
 * Description        : Appends the provided content to the specified file..
 ***********************************************************************************/
int append2file(const char* file_path, const char* content) {
    if (access(file_path, F_OK) == -1)
        return -1;

    FILE* file = fopen(file_path, "a");

    if (file == NULL)
        return -1;

    fprintf(file, "%s\n", content);
    fclose(file);
    return 0;
}

/***********************************************************************************
 * Function Name      : write2file
 * Inputs             : file_path (const char *) - path to the file
 *                      content (const char *) - content to write
 * Outputs            : None
 * Returns            : int - 0 if write successful
 *                           -1 if file does not exist or inaccessible
 * Description        : Write the provided content to the specified file.
 ***********************************************************************************/
int write2file(const char* file_path, const char* content) {
    if (access(file_path, F_OK) == -1)
        return -1;

    FILE* file = fopen(file_path, "w");

    if (file == NULL)
        return -1;

    fprintf(file, "%s\n", content);
    fclose(file);
    return 0;
}

/***********************************************************************************
 * Function Name      : log_encore
 * Inputs             : message (const char *) - message to log
 *                      ... (variadic arguments) - additional arguments for message
 * Outputs            : None
 * Returns            : None
 * Description        : print and logs a formatted message with a timestamp
 *                      to a log file.
 ***********************************************************************************/
void log_encore(const char* message, ...) {
    char* timestamp = timern();
    if (timestamp != NULL) {
        char logMesg[MAX_OUTPUT_LENGTH];
        va_list args;
        va_start(args, message);
        vsnprintf(logMesg, sizeof(logMesg), message, args);
        va_end(args);

        char logEncore[MAX_OUTPUT_LENGTH];
        snprintf(logEncore, sizeof(logEncore), "[%s] %s", timestamp, logMesg);
        if (append2file(LOG_FILE, logEncore) == -1)
            printf("[%s] error: encore_log file is inaccessible!\n", timestamp);

        free(timestamp);
    }
}

/***********************************************************************************
 * Function Name      : signal_handler
 * Inputs             : int signal - exit signal
 * Outputs            : None
 * Returns            : None
 * Description        : Handle SIGTERM and SIGINT signal.
 ***********************************************************************************/
static inline void signal_handler(int signal) {
    if (signal == SIGTERM) {
        log_encore("error: received SIGTERM.");
    } else if (signal == SIGINT) {
        log_encore("error: received SIGINT.");
    }

    // Exit gracefully
    exit(EXIT_SUCCESS);
}

/***********************************************************************************
 * Function Name      : execute_command
 * Inputs             : command (const char *) - shell command to execute
 * Outputs            : None
 * Returns            : char * - pointer to the dynamically allocated output of
 *                      the command execution
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

    FILE* fp;
    char buffer[MAX_OUTPUT_LENGTH];
    char* result = NULL;
    size_t result_length = 0;

    fp = popen(command, "r");
    if (fp == NULL) {
        log_encore("error: unable to exec command '%s'", command);
        return NULL;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        size_t buffer_length = strlen(buffer);
        char* new_result = realloc(result, result_length + buffer_length + 1);
        if (new_result == NULL) {
            log_encore("error: memory allocation error in execute_command()");
            free(result);
            pclose(fp);
            return NULL;
        }
        result = new_result;
        strcpy(result + result_length, buffer);
        result_length += buffer_length;
    }

    if (result != NULL)
        result[result_length] = '\0';

    if (pclose(fp) == -1)
        log_encore("error: closing command stream in execute_command()");

    return result;
}

/***********************************************************************************
 * Function Name      : systemv
 * Inputs             : format (const char *) - shell command to execute
 *                      variadic arguments - other arguments
 * Outputs            : None
 * Returns            : int - 0 if execution success
 *                           -1 if execution failed
 * Description        : Executes a shell command using system().
 ***********************************************************************************/
static inline int systemv(const char* format, ...) {
    if (format == NULL)
        return -1;

    char command[MAX_OUTPUT_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(command, sizeof(command), format, args);
    va_end(args);

    return system(command);
}

/***********************************************************************************
 * Function Name      : drm_fail
 * Inputs             : None
 * Outputs            : None
 * Returns            : None
 * Description        : Handle daemon exit and DRM message from drm_check function
 ***********************************************************************************/
static inline void drm_fail(void) {
    system("/system/bin/am start -a android.intent.action.VIEW -d \"https://encore.rem01gaming.dev/\" >/dev/null");
    system("su -lp 2000 -c \"/system/bin/cmd notification post -t 'Encore Tweaks' 'encore' 'DRM Check failed, please re-install "
           "Encore Tweaks from official website encore.rem01gaming.dev.'\" >/dev/null");
    log_encore("error: DRM Check failed, exiting.");
}

/***********************************************************************************
 * Function Name      : drm_check
 * Inputs             : None
 * Outputs            : None
 * Returns            : None
 * Description        : Check if module is Genuine and/or not modified by third party,
 *                      if modification detected, stop operation and forward user to
 *                      official website.
 ***********************************************************************************/
void drm_check(void) {
    // Check moduleid and service executable name
    if (access("/data/adb/modules/encore/system/bin/encored", F_OK) == -1) {
        drm_fail();
        exit(EXIT_FAILURE);
    }

    if (access("/data/adb/modules/encore/system/bin/encore_profiler", F_OK) == -1) {
        drm_fail();
        exit(EXIT_FAILURE);
    }

    if (access("/data/adb/modules/encore/system/bin/encore_utility", F_OK) == -1) {
        drm_fail();
        exit(EXIT_FAILURE);
    }

    if (systemv("sha256sum %s | grep -q %s", MODULE_PROP, MODULE_CHECKSUM) != 0) {
        drm_fail();
        exit(EXIT_FAILURE);
    }
}

/***********************************************************************************
 * Function Name      : set_priority
 * Inputs             : pid (const char *) - PID as a string
 * Outputs            : None
 * Returns            : None
 * Description        : Sets the CPU nice priority of a given process.
 ***********************************************************************************/
static inline void set_priority(const char* pid) {
    pid_t process_id = atoi(pid);
    log_encore("info: priority settings for PID %s", pid);
    if (setpriority(PRIO_PROCESS, process_id, -20) == -1)
        log_encore("error: unable to set nice priority for %s", pid);
}

/***********************************************************************************
 * Function Name      : preload_game
 * Inputs             : gamestart (const char *) - Package name of the game
 * Outputs            : None
 * Returns            : None
 * Description        : Lock game shader cache into memory using vmtouch.
 ***********************************************************************************/
static inline void preload_game(const char* gamestart) {
    if (system("cat /data/encore/game_preload | grep -q 1") != -1) {
        systemv("su -c vmtouch -ld /sdcard/Android/data/%s/cache/vulkan_pso_cache.bin /sdcard/Android/data/%s/cache/UnityShaderCache "
                "/sdcard/Android/data/%s/files/ProgramBinaryCache",
                gamestart, gamestart, gamestart);
    }
}

/***********************************************************************************
 * Function Name      : perf_common
 * Inputs             : None
 * Outputs            : None
 * Returns            : None
 * Description        : Executes a command to apply common performance settings.
 ***********************************************************************************/
static inline void perf_common(void) {
    write2file("/dev/encore_mode", "perfcommon");
    system("su -c encore_profiler");
}

/***********************************************************************************
 * Function Name      : performance_mode
 * Inputs             : None
 * Outputs            : None
 * Returns            : None
 * Description        : Executes a command to switch to performance profile.
 ***********************************************************************************/
static inline void performance_mode(void) {
    drm_check();
    write2file("/dev/encore_mode", "performance");
    system("su -c encore_profiler");
}

/***********************************************************************************
 * Function Name      : normal_mode
 * Inputs             : None
 * Outputs            : None
 * Returns            : None
 * Description        : Executes a command to switch to normal profile.
 ***********************************************************************************/
static inline void normal_mode(void) {
    drm_check();
    write2file("/dev/encore_mode", "normal");
    system("su -c encore_profiler");
}

/***********************************************************************************
 * Function Name      : powersave_mode
 * Inputs             : None
 * Outputs            : None
 * Returns            : None
 * Description        : Executes a command to switch to performance profile.
 ***********************************************************************************/
static inline void powersave_mode(void) {
    drm_check();
    write2file("/dev/encore_mode", "powersave");
    system("su -c encore_profiler");
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
 *                      listed in gamelist.txt.
 * Note               : Caller is responsible for freeing the returned string.
 ***********************************************************************************/
static inline char* get_gamestart(void) {
    char* gamestart = execute_command("dumpsys window visible-apps | grep 'package=.* ' | grep -Eo "
                                      "$(cat /data/encore/gamelist.txt)");
    return trim_newline(gamestart);
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
    char* state = execute_command("su -c dumpsys power | grep -Eo 'mWakefulness=Awake|mWakefulness=Asleep' "
                                  "| awk -F'=' '{print $2}'");
    if (state == NULL) {
        state = execute_command("su -c dumpsys window displays | grep -Eo 'mAwake=true|mAwake=false' | "
                                "awk -F'=' '{print $2}'");
    }
    return trim_newline(state);
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
    char* low_power = execute_command("su -c dumpsys power | grep -Eo "
                                      "'mSettingBatterySaverEnabled=true|mSettingBatterySaverEnabled=false' | "
                                      "awk -F'=' '{print $2}'");
    if (low_power == NULL)
        low_power = execute_command("su -c settings get global low_power");

    return trim_newline(low_power);
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
    char* pid = execute_command("pidof %s", name);
    return trim_newline(pid);
}

/***********************************************************************************
 * Function Name      : notify_toast
 * Inputs             : message (char *) - Message to display
 * Outputs            : None
 * Returns            : None
 * Description        : Sends a command to start a toast notification with
 *                      your message. Uses the `am start` command to trigger
 *                      a toast via the bellavita.toast MainActivity.
 ***********************************************************************************/
static inline void notify_toast(const char* message) {
    systemv("/system/bin/am start -a android.intent.action.MAIN -e toasttext \"%s\" -n bellavita.toast/.MainActivity >/dev/null",
            message);
}

/***********************************************************************************
 * Function Name      : handle_mlbb
 * Inputs             : const char *gamestart - Game package name
 * Outputs            : None
 * Returns            : int - 2 if MLBB is running in foreground
 *                            1 if MLBB is running in background
 *                            0 if gamestart is not MLBB
 *                           -1 if gamestart is NULL
 * Description        : Checks if "com.mobile.legends" IS actually running
 *                      on foreground, not in the background.
 ***********************************************************************************/
static inline int handle_mlbb(const char* gamestart) {
    if (gamestart == NULL)
        return -1;

    if (strcmp(gamestart, "com.mobile.legends") != 0)
        return 0;

    if (system("pidof com.mobile.legends:UnityKillsMe >/dev/null") == 0)
        return 2;

    return 1;
}

int main(void) {
    // DRM Check
    drm_check();

    // Daemonize service
    if (daemon(0, 0) != 0) {
        log_encore("error: unable to daemonize service");
        exit(EXIT_FAILURE);
    }

    // Register signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    char *gamestart = NULL, *screenstate = NULL, *low_power = NULL, *pid = NULL, mlbb_is_running = 0, cur_mode = -1,
         path[MAX_PATH_LENGTH];
    log_encore("info: daemon started");
    perf_common();

    while (1) {
        free(screenstate);
        if (low_power) {
            free(low_power);
            low_power = NULL;
        }

        sleep(15);

        if (gamestart == NULL) {
            // Only fetch gamestart and low_power state when user not in-game
            // prevent overhead from dumpsys commands.
            gamestart = get_gamestart();
            low_power = get_low_power_state();
        } else {
            // Check if the game is still running
            snprintf(path, sizeof(path), "/proc/%s", pid);
            if (access(path, F_OK) == -1) {
                free(pid);
                pid = NULL;
                free(gamestart);
                gamestart = get_gamestart();
                low_power = get_low_power_state();
                system("su -c pkill vmtouch");
            }
        }

        screenstate = get_screenstate();
        mlbb_is_running = handle_mlbb(gamestart);

        // Handle in case screenstate is empty
        if (screenstate == NULL) {
            log_encore("error: unable to get current screenstate, service won't work properly!");
            sleep(30);
            continue;
        }

        if (gamestart && (strcmp(screenstate, "Awake") == 0 || strcmp(screenstate, "true") == 0) && mlbb_is_running != 1) {
            // Bail out if we already on performance profile
            if (cur_mode == 1)
                continue;

            // Get PID and check if the game is "real" running program
            pid = pidof(gamestart);
            if (pid == NULL) {
                log_encore("error: unable to fetch PID of %s", gamestart);
                continue;
            }

            // Handle weird behavior of MLBB
            if (mlbb_is_running == 2)
                pid = pidof("com.mobile.legends:UnityKillsMe");

            cur_mode = 1;
            log_encore("info: applying performance profile for %s", gamestart);
            notify_toast("Applying performance profile...");
            performance_mode();
            set_priority(pid);
            preload_game(gamestart);
        } else if (low_power && (strcmp(low_power, "true") == 0 || strcmp(low_power, "1") == 0)) {
            // Bail out if we already on powersave profile
            if (cur_mode == 2)
                continue;

            cur_mode = 2;
            log_encore("info: applying powersave profile");
            notify_toast("Applying powersave profile...");
            powersave_mode();
        } else {
            // Bail out if we already on normal profile
            if (cur_mode == 0)
                continue;

            cur_mode = 0;
            log_encore("info: applying normal profile");
            notify_toast("Applying normal profile...");
            normal_mode();
        }
    }

    return 0;
}
