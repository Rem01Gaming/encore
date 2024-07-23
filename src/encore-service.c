#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <sys/stat.h>
#include <unistd.h>

#define MAX_OUTPUT_LENGTH 1024
#define MAX_COMMAND_LENGTH 512

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

/* void write2file(const char *file_path, const char *content) {
  if (access(file_path, F_OK) != -1) {
    chmod(file_path, 0644);
    FILE *file = fopen(file_path, "w");
    if (file != NULL) {
      fprintf(file, "%s\n", content);
      fclose(file);
      chmod(file_path, 0444);
    } else {
      printf("error: can't open %s\n", file_path);
    }
  } else {
    printf("error: %s does not exist nor not accessible\n", file_path);
  }
} */

void setPriorities(const char *pid) {
  snprintf(command, sizeof(command), "su -c encore-setpriority %s", pid);
  system(command);
}

void perf_common(void) { system("su -c encore-perfcommon"); }

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

  perf_common();

  while (1) {
    if (!gamestart) {
      gamestart =
          execute_command("sh /data/encore/AppMonitoringUtil.sh | head -n 1");
    } else {
      snprintf(path, sizeof(path), "/proc/%s", trim_newline(pid));
      if (access(path, F_OK) == -1) {
        free(pid);
        pid = NULL;
        free(gamestart);
        gamestart = NULL;
        gamestart =
            execute_command("sh /data/encore/AppMonitoringUtil.sh | head -n 1");
      }
    }

    screenstate = execute_command(
        "su -c dumpsys window displays | grep -Eo "
        "\"mAwake=false|mAwake=true\" "
        "| awk -F'=' '{print $2}'");
    low_power = execute_command(
        "su -c dumpsys power | grep -Eo "
        "\"mSettingBatterySaverEnabled=true|mSettingBatterySaverEnabled="
        "false\" | awk -F'=' '{print $2}'");

    if (screenstate == NULL) {
      printf("error: screenstate is null\n");
    } else if (gamestart && strcmp(trim_newline(screenstate), "true") == 0) {
      // Apply performance mode
      if (cur_mode != 1) {
        cur_mode = 1;
        printf("Applying performance mode\n");
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
          printf("error: Game PID is null, can't set priority\n");
        }
      }
    } else if (low_power && strcmp(trim_newline(low_power), "true") == 0) {
      // Apply powersave mode
      if (cur_mode != 2) {
        cur_mode = 2;
        printf("Applying powersave mode\n");
        powersave_mode();
      }
    } else {
      // Apply normal mode
      if (cur_mode != 0) {
        cur_mode = 0;
        printf("Applying normal mode\n");
        normal_mode();
      }
    }

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

    sleep(12);
  }

  return 0;
}
