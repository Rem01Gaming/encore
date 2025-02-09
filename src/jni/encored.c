#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define LOG_FILE "/data/encore/encore_log"
#define GAMELIST "/data/encore/gamelist.txt"
#define MODULE_PROP "/data/adb/modules/encore/module.prop"
#define MODULE_UPDATE "/data/adb/modules/encore/update"
#define GAME_STRESS "com.mobile.legends:UnityKillsMe"
#define MAX_COMMAND_LENGTH 1024
#define MAX_OUTPUT_LENGTH 150

#define IS_AWAKE(state) (strcmp(state, "Awake") == 0 || strcmp(state, "true") == 0)
#define IS_LOW_POWER(state) (strcmp(state, "true") == 0 || strcmp(state, "1") == 0)

#define MY_PATH                                                                                                                    \
    "PATH=/system/bin:/system/xbin:/data/adb/ap/bin:/data/adb/ksu/bin:/data/adb/magisk:/debug_ramdisk:/sbin:/sbin/su:/su/bin:/su/" \
    "xbin:/data/data/com.termux/files/usr/bin"

typedef enum { PERFCOMMON = 0, PERFORMANCE_PROFILE = 1, NORMAL_PROFILE = 2, POWERSAVE_PROFILE = 3 } ProfileMode;
typedef enum { MLBB_NOT_RUNNING = 0, MLBB_RUN_BG = 1, MLBB_RUNNING = 2 } MLBBState;

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
 * Function Name      : write2file
 * Inputs             : file_path (const char *) - path to the file
 *                      content (const char *) - content to write
 *                      mode (const int) - 1 for append and 0 for write
 * Outputs            : None
 * Returns            : int - 0 if write successful
 *                           -1 if file does not exist or inaccessible
 * Description        : Write the provided content to the specified file.
 ***********************************************************************************/
static inline int write2file(const char* file_path, const char* content, const int mode) {
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
        if (write2file(LOG_FILE, logEncore, 1) == -1)
            printf("[%s] error: encore_log file is inaccessible!\n", timestamp);

        free(timestamp);
    }
}

/***********************************************************************************
 * Function Name      : sighandler
 * Inputs             : int signal - exit signal
 * Outputs            : None
 * Returns            : None
 * Description        : Handle exit signal.
 ***********************************************************************************/
static inline void sighandler(const int signal) {
    switch (signal) {
    case SIGTERM:
        log_encore("notice: received SIGTERM.");
        break;
    case SIGINT:
        log_encore("notice: received SIGINT.");
        break;
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

    putenv(MY_PATH);
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

    char command[MAX_COMMAND_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(command, sizeof(command), format, args);
    va_end(args);

    putenv(MY_PATH);
    return system(command);
}

/***********************************************************************************
 * Function Name      : notify
 * Inputs             : message (char *) - Message to display
 * Outputs            : None
 * Returns            : int - 0 if success
 *                           -1 if failed
 * Description        : Sends a message to regular notification.
 ***********************************************************************************/
static inline int notify(const char* message) {
    return systemv("su -lp 2000 -c \"/system/bin/cmd notification post -t 'Encore Tweaks' 'encore' '%s'\" >/dev/null", message);
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
    if (pid == NULL)
        return;

    write2file("/dev/cpuctl/encore/tasks", pid, 0);
    write2file("/dev/stune/encore/tasks", pid, 0);
}

/***********************************************************************************
 * Function Name      : rewrite_module_prop
 * Inputs             : None
 * Outputs            : None
 * Returns            : None
 * Description        : Prevent 3rd party from renaming the module
 ***********************************************************************************/
static inline void rewrite_module_prop(void) {
    systemv("sed -i 's/name=.*/name=Encore Tweaks/' %s", MODULE_PROP);
    systemv("sed -i 's/author=.*/author=Rem01Gaming/' %s", MODULE_PROP);
}

/***********************************************************************************
 * Function Name      : run_profiler
 * Inputs             : int - 0 for perfcommon
 *                            1 for performance
 *                            2 for normal
 *                            3 for powersave
 * Outputs            : None
 * Returns            : int - 0 if execution success
 *                           -1 if execution failed
 * Description        : Executes a command to switch to performance profile.
 ***********************************************************************************/
static inline int run_profiler(const int profile) {
    char profile_str[16];
    snprintf(profile_str, sizeof(profile_str), "%d", profile);
    write2file("/dev/encore_mode", profile_str, 0);
    rewrite_module_prop();
    return systemv("encore_profiler %d", profile);
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
    char* gamestart = execute_command("dumpsys window visible-apps | grep 'package=.* ' | grep -Eo -f %s", GAMELIST);
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
    char* state = execute_command("dumpsys power | grep -Eo 'mWakefulness=Awake|mWakefulness=Asleep' "
                                  "| awk -F'=' '{print $2}'");
    if (state == NULL) {
        state = execute_command("dumpsys window displays | grep -Eo 'mAwake=true|mAwake=false' | "
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
    char* low_power = execute_command("settings get global low_power");
    if (low_power == NULL) {
        low_power = execute_command("dumpsys power | grep -Eo "
                                    "'mSettingBatterySaverEnabled=true|mSettingBatterySaverEnabled=false' | "
                                    "awk -F'=' '{print $2}'");
    }
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
    char* pid = execute_command("toybox pidof %s || busybox pidof %s || pidof %s", name, name, name);
    return trim_newline(pid);
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

    if (systemv("toybox pidof %s || busybox pidof %s || pidof %s", GAME_STRESS, GAME_STRESS, GAME_STRESS) == 0)
        return 2;

    return 1;
}

int main(void) {
    // Handle case when not running on root
    if (getuid() != 0) {
        fprintf(stderr, "Run it as root\n");
        exit(EXIT_FAILURE);
    }

    // Handle case when module ID is not 'encore'
    if (access(MODULE_PROP, F_OK) != 0) {
        log_encore("error: critical file not found (%s)", MODULE_PROP);
        exit(EXIT_FAILURE);
    }

    // Handle missing Gamelist
    if (access(GAMELIST, F_OK) != 0) {
        log_encore("error: critical file not found (%s)", GAMELIST);
        exit(EXIT_FAILURE);
    }

    // Daemonize service
    if (daemon(0, 0)) {
        log_encore("error: unable to daemonize service");
        exit(EXIT_FAILURE);
    }

    // Register signal handlers
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    // Initialize variables
    char *gamestart = NULL, *screenstate = NULL, *low_power = NULL, *pid = NULL;
    MLBBState mlbb_is_running = MLBB_NOT_RUNNING;
    ProfileMode cur_mode = -1;

    log_encore("info: daemon started");
    run_profiler(PERFCOMMON); // exec perfcommon

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
        } else if (pid && kill(atoi(pid), 0) == -1) {
            free(pid);
            pid = NULL;
            free(gamestart);
            gamestart = get_gamestart();
            low_power = get_low_power_state();
        }

        screenstate = get_screenstate();

        if (gamestart != NULL)
            mlbb_is_running = handle_mlbb(gamestart);

        // Handle in case screenstate is empty
        if (screenstate == NULL) {
            log_encore("error: unable to get current screenstate, service won't work properly!");
            sleep(30);
            continue;
        }

        // Handle case when module gets updated
        if (access(MODULE_UPDATE, F_OK) == 0) {
            log_encore("notice: module update detected, exiting.");
            notify("Please reboot your device to complete module update.");
            exit(EXIT_SUCCESS);
        }

        if (gamestart && IS_AWAKE(screenstate) && mlbb_is_running != MLBB_RUN_BG) {
            // Bail out if we already on performance profile
            if (cur_mode == PERFORMANCE_PROFILE)
                continue;

            // Get PID and check if the game is "real" running program
            pid = pidof(gamestart);
            if (pid == NULL) {
                log_encore("error: unable to fetch PID of %s", gamestart);
                continue;
            }

            // Handle weird behavior of MLBB
            if (mlbb_is_running == MLBB_RUNNING)
                pid = pidof(GAME_STRESS);

            cur_mode = PERFORMANCE_PROFILE;
            log_encore("info: applying performance profile for %s", gamestart);
            run_profiler(PERFORMANCE_PROFILE);
            set_priority(pid);
        } else if (low_power && IS_LOW_POWER(low_power)) {
            // Bail out if we already on powersave profile
            if (cur_mode == POWERSAVE_PROFILE)
                continue;

            cur_mode = POWERSAVE_PROFILE;
            log_encore("info: applying powersave profile");
            run_profiler(POWERSAVE_PROFILE);
        } else {
            // Bail out if we already on normal profile
            if (cur_mode == NORMAL_PROFILE)
                continue;

            cur_mode = NORMAL_PROFILE;
            log_encore("info: applying normal profile");
            run_profiler(NORMAL_PROFILE);
        }
    }

    return 0;
}
