#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define MAX_OUTPUT_LENGTH 128
#define MAX_COMMAND_LENGTH 256

char command[MAX_COMMAND_LENGTH];
char path[256];

char *trim_newline(char *str) {
  if (str == NULL) return NULL;
  char *end;
  if ((end = strchr(str, '\n')) != NULL) {
    *end = '\0';
  }
  return str;
}

char *timern(void) {
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  char *s = malloc(64 * sizeof(char));
  if (s == NULL) {
    printf("error: memory allocation failed in timern()\n");
    return NULL;
  }
  size_t ret = strftime(s, 64, "%c", tm);
  if (ret == 0) {
    printf("error: strftime failed in timern()\n");
    free(s);
    return NULL;
  }
  return s;
}

char *execute_command(const char *command) {
  FILE *fp;
  char buffer[MAX_OUTPUT_LENGTH];
  char *result = NULL;
  size_t result_length = 0;

  fp = popen(command, "r");
  if (fp == NULL) {
    printf("error: can't exec command %s\n", command);
    return NULL;
  }

  while (fgets(buffer, sizeof(buffer), fp) != NULL) {
    size_t buffer_length = strlen(buffer);
    char *new_result = realloc(result, result_length + buffer_length + 1);
    if (new_result == NULL) {
      printf("error: memory allocation error.\n");
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
    printf("error: closing command stream.\n");
  }

  return result;
}

void append2file(const char *file_path, const char *content) {
  if (access(file_path, F_OK) != -1) {
    chmod(file_path, 0644);
    FILE *file = fopen(file_path, "a");
    if (file != NULL) {
      fprintf(file, "%s\n", content);
      fclose(file);
      chmod(file_path, 0444);
    } else {
      printf("error: can't open %s\n", file_path);
    }
  } else {
    printf("error: %s does not exist or is not accessible\n", file_path);
  }
}

void log_encore(const char *message, ...) {
  char *timestamp = timern();
  if (timestamp != NULL) {
    char logMesg[300];
    va_list args;
    va_start(args, message);
    vsnprintf(logMesg, sizeof(logMesg), message, args);
    va_end(args);

    char logEncore[512];
    snprintf(logEncore, sizeof(logEncore), "[%s] %s", timestamp, logMesg);
    append2file("/data/encore/encore_log", logEncore);

    free(timestamp);
  }
}

void setPriorities(const char *pid) {
  int prio = -20;    // Niceness
  int io_class = 1;  // I/O class
  int io_prio = 0;   // I/O priority

  pid_t process_id = atoi(pid);

  if (setpriority(PRIO_PROCESS, process_id, prio) == -1) {
    printf("Failed to set nice priority for %s", pid);
    log_encore("error: failed to set nice priority for %s", pid);
  }

  if (syscall(SYS_ioprio_set, 1, process_id, (io_class << 13) | io_prio) == -1) {
    printf("error: failed to set IO priority for %s", pid);
    log_encore("error: ailed to set IO priority for %s", pid);
  }
}

void performance_mode(void) { system("su -c encore-performance"); }

void normal_mode(void) { system("su -c encore-normal"); }

void powersave_mode(void) {
  normal_mode();
  system("su -c encore-powersave");
}

int main(void) {
  char *gamestart = NULL;
  char *screenstate = NULL;
  char *low_power = NULL;
  char *pid = NULL;
  int cur_mode = -1;

  while (1) {
    /* Run app monitoring ONLY if we aren't on performance profile, prevent
     * massive overhead while gaming */
    if (!gamestart) {
      gamestart =
          execute_command("dumpsys window displays | grep -E 'mCurrentFocus' | grep -Eo $(cat /data/encore/gamelist.txt)");
      low_power = execute_command(
          "su -c dumpsys power | grep -Eo "
          "\"mSettingBatterySaverEnabled=true|mSettingBatterySaverEnabled="
          "false\" | awk -F'=' '{print $2}'");
    } else {
      snprintf(path, sizeof(path), "/proc/%s", trim_newline(pid));
      if (access(path, F_OK) == -1) {
        free(pid);
        pid = NULL;
        free(gamestart);
        gamestart = NULL;
        gamestart =
            execute_command("dumpsys window displays | grep -E 'mCurrentFocus' | grep -Eo $(cat /data/encore/gamelist.txt)");
      }
    }

    screenstate = execute_command(
        "su -c dumpsys power | grep -Eo "
        "'mWakefulness=Awake|mWakefulness=Asleep' | awk -F'=' '{print $2}'");

    /* In some cases, some device fails to give mWakefulness info. */
    if (screenstate == NULL) {
      screenstate = execute_command(
          "su -c dumpsys window displays | grep -Eo 'mAwake=true|mAwake=false' "
          "| awk -F'=' '{print $2}'");
    }

    // Handle null screenstate
    if (screenstate == NULL) {
      printf("error: screenstate is null!\n");
      log_encore("error: screenstate is null!");
    } else if (gamestart && (strcmp(trim_newline(screenstate), "Awake") == 0 ||
                             strcmp(trim_newline(screenstate), "true") == 0)) {
      // Apply performance mode
      if (cur_mode != 1) {
        cur_mode = 1;
        printf("Applying performance profile for %s", trim_newline(gamestart));
        log_encore("info: applying performance profile for %s", trim_newline(gamestart));
        snprintf(
            command, sizeof(command),
            "/system/bin/am start -a android.intent.action.MAIN -e toasttext "
            "\"Boosting game %s\" -n bellavita.toast/.MainActivity",
            trim_newline(gamestart));
        system(command);
        performance_mode();

        snprintf(command, sizeof(command), "pidof %s", trim_newline(gamestart));
        pid = execute_command(command);
        if (pid != NULL) {
          setPriorities(trim_newline(pid));
        } else {
          printf("error: could not fetch pid of %s\n", trim_newline(pid));
          log_encore("error: could not fetch pid of %s\n", trim_newline(pid));
        }
      }
    } else if (low_power && strcmp(trim_newline(low_power), "true") == 0) {
      // Apply powersave mode
      if (cur_mode != 2) {
        cur_mode = 2;
        printf("Applying powersave profile\n");
        log_encore("info: applying powersave profile");
        powersave_mode();
      }
    } else {
      // Apply normal mode
      if (cur_mode != 0) {
        cur_mode = 0;
        printf("Applying normal profile\n");
        log_encore("info: applying normal profile");
        normal_mode();
      }
    }

    // Print info to console
    if (gamestart) {
      printf("gamestart: %s\n", trim_newline(gamestart));
    } else {
      printf("gamestart: NULL\n");
    }
    if (screenstate) {
      printf("screenstate: %s\n", trim_newline(screenstate));
      free(screenstate);
      screenstate = NULL;
    } else {
      printf("screenstate: NULL\n");
    }
    if (low_power) {
      printf("low_power: %s\n", trim_newline(low_power));
      free(low_power);
      low_power = NULL;
    } else {
      printf("low_power: NULL\n");
    }

    sleep(15);
  }

  return 0;
}
