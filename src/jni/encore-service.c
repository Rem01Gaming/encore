#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define MAX_OUTPUT_LENGTH 128
#define MAX_COMMAND_LENGTH 172

char command[MAX_COMMAND_LENGTH];
char path[64];

/***********************************************************************************
 * Function Name      : trim_newline
 * Inputs             : str (char *) - string to trim newline from
 * Outputs            : str (char *) - string without newline
 * Returns            : char * - pointer to the modified string
 * Description        : Trims a newline character at the end of a string if
 *                      present.
 ***********************************************************************************/
char *trim_newline(char *string) {
  if (string == NULL) return NULL;
  char *end;
  if ((end = strchr(string, '\n')) != NULL) {
    *end = '\0';
  }
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
char *timern(void) {
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  char *timestamp = malloc(64 * sizeof(char));
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
 * Returns            : None
 * Description        : Appends the provided content to the specified file.
 *                      if the file does not exist, an error message is printed.
 ***********************************************************************************/
void append2file(const char *file_path, const char *content) {
  if (access(file_path, F_OK) != -1) {
    FILE *file = fopen(file_path, "a");
    if (file != NULL) {
      fprintf(file, "%s\n", content);
      fclose(file);
    } else {
      printf("error: can't open %s\n", file_path);
    }
  } else {
    printf("error: %s does not exist or inaccessible\n", file_path);
  }
}

/***********************************************************************************
 * Function Name      : log_encore
 * Inputs             : message (const char *) - message to log
 *                      ... (variadic arguments) - additional arguments for message
 * Outputs            : None
 * Returns            : None
 * Description        : print and logs a formatted message with a timestamp
 *                      to a log file ("/data/encore/encore_log").
 ***********************************************************************************/
void log_encore(const char *message, ...) {
  char *timestamp = timern();
  if (timestamp != NULL) {
    char logMesg[MAX_OUTPUT_LENGTH];
    va_list args;
    va_start(args, message);
    vsnprintf(logMesg, sizeof(logMesg), message, args);
    va_end(args);

    char logEncore[MAX_OUTPUT_LENGTH];
    snprintf(logEncore, sizeof(logEncore), "[%s] %s", timestamp, logMesg);
    printf("%s\n", logEncore);
    append2file("/data/encore/encore_log", logEncore);

    free(timestamp);
  }
}

/***********************************************************************************
 * Function Name      : execute_command
 * Inputs             : command (const char *) - shell command to execute
 * Outputs            : None
 * Returns            : char * - pointer to the dynamically allocated output of
 *                      the command execution
 * Description        : Executes a shell command and captures its output.
 ***********************************************************************************/
char *execute_command(const char *command) {
  FILE *fp;
  char buffer[MAX_OUTPUT_LENGTH];
  char *result = NULL;
  size_t result_length = 0;

  fp = popen(command, "r");
  if (fp == NULL) {
    log_encore("error: can't exec command '%s'", command);
    return NULL;
  }

  while (fgets(buffer, sizeof(buffer), fp) != NULL) {
    size_t buffer_length = strlen(buffer);
    char *new_result = realloc(result, result_length + buffer_length + 1);
    if (new_result == NULL) {
      printf("error: memory allocation error in execute_command()\n");
      free(result);
      pclose(fp);
      return NULL;
    }
    result = new_result;
    strcpy(result + result_length, buffer);
    result_length += buffer_length;
  }

  if (result != NULL) {
    result[result_length] = '\0';
  }

  if (pclose(fp) == -1) {
    printf("error: closing command stream in execute_command()");
  }

  return result;
}

/***********************************************************************************
 * Function Name      : set_priority
 * Inputs             : pid (const char *) - PID as a string
 * Outputs            : None
 * Returns            : None
 * Description        : Sets the CPU nice priority and I/O priority of a given
 *                      process.
 ***********************************************************************************/
void set_priority(const char *pid) {
  int prio = -20;    // Niceness
  int io_class = 1;  // I/O class
  int io_prio = 0;   // I/O priority

  pid_t process_id = atoi(pid);
  log_encore("info: priority settings for PID %s", pid);

  if (setpriority(PRIO_PROCESS, process_id, prio) == -1) {
    log_encore("error: failed to set nice priority for %s", pid);
  }

  if (syscall(SYS_ioprio_set, 1, process_id, (io_class << 13) | io_prio) ==
      -1) {
    log_encore("error: failed to set IO priority for %s", pid);
  }
}

/***********************************************************************************
 * Function Name      : perf_common
 * Inputs             : None
 * Outputs            : None
 * Returns            : None
 * Description        : Executes a command to apply common performance settings.
 ***********************************************************************************/
void perf_common(void) {
  log_encore("info: service started, applying perfcommon...");
  system("su -c encore-perfcommon");
}

/***********************************************************************************
 * Function Name      : performance_mode
 * Inputs             : None
 * Outputs            : None
 * Returns            : None
 * Description        : Executes a command to switch to performance mode.
 ***********************************************************************************/
void performance_mode(void) { system("su -c encore-performance"); }

/***********************************************************************************
 * Function Name      : normal_mode
 * Inputs             : None
 * Outputs            : None
 * Returns            : None
 * Description        : Executes a command to switch to normal mode.
 ***********************************************************************************/
void normal_mode(void) { system("su -c encore-normal"); }

/***********************************************************************************
 * Function Name      : powersave_mode
 * Inputs             : None
 * Outputs            : None
 * Returns            : None
 * Description        : Executes commands to switch to powersave mode by first
 *                      applying normal settings and then powersave-specific settings.
 ***********************************************************************************/
void powersave_mode(void) {
  normal_mode();
  system("su -c encore-powersave");
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
char *get_gamestart(void) {
  return execute_command(
      "dumpsys window visible-apps | grep 'package=.* ' | grep -Eo "
      "$(cat /data/encore/gamelist.txt)");
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
char *get_screenstate(void) {
  char *state = execute_command(
      "su -c dumpsys power | grep -Eo 'mWakefulness=Awake|mWakefulness=Asleep' "
      "| awk -F'=' '{print $2}'");
  if (state == NULL) {
    state = execute_command(
        "su -c dumpsys window displays | grep -Eo 'mAwake=true|mAwake=false' | "
        "awk -F'=' '{print $2}'");
  }
  return state;
}

/***********************************************************************************
 * Function Name      : get_low_power_state
 * Inputs             : None
 * Outputs            : None
 * Returns            : char* ("true" if Battery Saver is enabled, "false" otherwise)
 * Description        : Checks if the device's Battery Saver mode is enabled by using 
 *                      dumpsys power and filtering for the battery saver status.
 *                      Useful for determining low-power states.
 * Note               : Caller is responsible for freeing the returned string.
 ***********************************************************************************/
char *get_low_power_state(void) {
  return execute_command(
      "su -c dumpsys power | grep -Eo "
      "'mSettingBatterySaverEnabled=true|mSettingBatterySaverEnabled=false' | "
      "awk -F'=' '{print $2}'");
}

/***********************************************************************************
 * Function Name      : notify_game
 * Inputs             : const char* gamestart (name of the game package to boost)
 * Outputs            : None
 * Returns            : None
 * Description        : Sends a command to start a toast notification indicating that 
 *                      the specified game is being boosted.
 *                      Uses the `am start` command to trigger a toast via 
 *                      the bellavita.toast MainActivity.
 *                      Useful for providing user feedback.
 ***********************************************************************************/
void notify_game(const char *gamestart) {
  snprintf(command, sizeof(command),
           "/system/bin/am start -a android.intent.action.MAIN -e toasttext "
           "\"Boosting game %s\" -n bellavita.toast/.MainActivity >/dev/null",
           gamestart);
  system(command);
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
int handle_mlbb(const char *gamestart) {
  if (gamestart == NULL) {
    return -1;
  }

  if (strcmp(gamestart, "com.mobile.legends") != 0) {
    return 0;
  }

  if (system("pidof com.mobile.legends:UnityKillsMe >/dev/null") == 0) {
    return 2;
  }

  return 1;
}

int main(void) {
  char *gamestart = NULL, *screenstate = NULL, *low_power = NULL, *pid = NULL, mlbb_is_running = 0, cur_mode = -1;

  perf_common();

  while (1) {
    // Run app monitoring if not on performance profile
    if (gamestart == NULL) {
      gamestart = get_gamestart();
      low_power = get_low_power_state();
    } else {
      snprintf(path, sizeof(path), "/proc/%s", trim_newline(pid));
      if (access(path, F_OK) == -1) {
        free(pid);
        pid = NULL;
        free(gamestart);
        gamestart = get_gamestart();
      }
    }

    screenstate = get_screenstate();
    mlbb_is_running = handle_mlbb(trim_newline(gamestart));

    if (screenstate == NULL) {
      log_encore("error: failed to get current screenstate, service won't work properly!");
    } else if (gamestart && (strcmp(trim_newline(screenstate), "Awake") == 0 ||
                             strcmp(trim_newline(screenstate), "true") == 0)) {
      // Apply performance mode
      if (cur_mode != 1) {
        snprintf(command, sizeof(command), "pidof %s", trim_newline(gamestart));
        pid = execute_command(command);

        // Handle weird behavior of MLBB
        if (mlbb_is_running == 2) {
          pid = execute_command("pidof com.mobile.legends:UnityKillsMe");
          log_encore("info: MLBB detected, boosting UnityKillsMe thread");
        } else if (mlbb_is_running == 1) {
          log_encore("info: MLBB detected but UnityKillsMe thread is not running");
        }

        if (pid != NULL && mlbb_is_running != 1) {
          cur_mode = 1;
          log_encore("info: applying performance profile for %s",
                     trim_newline(gamestart));
          notify_game(trim_newline(gamestart));
          performance_mode();
          set_priority(trim_newline(pid));
        } else {
          log_encore("error: could not fetch pid of %s, can't start performance profile",
                     trim_newline(gamestart));
        }
      }
    } else if (low_power && strcmp(trim_newline(low_power), "true") == 0) {
      // Apply powersave mode
      if (cur_mode != 2) {
        cur_mode = 2;
        log_encore("info: applying powersave profile");
        powersave_mode();
      }
    } else {
      // Apply normal mode
      if (cur_mode != 0) {
        cur_mode = 0;
        log_encore("info: applying normal profile");
        normal_mode();
      }
    }

    /* Print info to console
    printf("gamestart: %s\n", gamestart ? trim_newline(gamestart) : "NULL");
    printf("screenstate: %s\n",
           screenstate ? trim_newline(screenstate) : "NULL");
    printf("low_power: %s\n", low_power ? trim_newline(low_power) : "NULL"); */

    free(screenstate);
    screenstate = NULL;

    if (low_power) {
      free(low_power);
      low_power = NULL;
    }

    sleep(15);
  }

  return 0;
}
