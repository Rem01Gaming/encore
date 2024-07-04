#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <sys/stat.h>
#include <unistd.h>

#define MAX_OUTPUT_LENGTH 1024
#define MAX_COMMAND_LENGTH 512

int cur_mode = -1;
char command[MAX_COMMAND_LENGTH];
char *gamestart = NULL;

char *trim_newline(char *str) {
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
    result = realloc(result, result_length + buffer_length + 1);
    if (result == NULL) {
      printf("error: memory allocation error.\n");
      pclose(fp);
      return NULL;
    }
    strcpy(result + result_length, buffer);
    result_length += buffer_length;
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
  snprintf(command, sizeof(command), "renice -n -20 -p %s", pid);
  system(command);

  snprintf(command, sizeof(command), "ionice -c 1 -n 0 -p %s", pid);
  system(command);

  snprintf(command, sizeof(command), "chrt -f -p 98 %s", pid);
  system(command);
}

void performance_mode(void) { system("sh /system/bin/encore-performance"); }

void normal_mode(void) { system("sh /system/bin/encore-normal"); }

void powersave_mode(void) {
  normal_mode();
  system("sh /system/bin/encore-powersave");
}

void perf_common(void) {
  system(
      "su -lp 2000 -c \"/system/bin/cmd notification post -S bigtext -t "
      "\\\"ENCORE\\\" \\\"Tag$(date +%s)\\\" \\\"Tweaks applied "
      "successfully\\\"\"");
  system("sh /system/bin/encore-perfcommon");
}

void apply_mode(const int mode) {
  char *pid = NULL;

  if (mode == 1 && cur_mode != 1) {
    cur_mode = 1;
    printf("Applying performance mode\n");
    snprintf(command, sizeof(command), "pidof %s", gamestart);
    pid = execute_command(command);
    setPriorities(trim_newline(pid));
    snprintf(command, sizeof(command),
             "/system/bin/am start -a android.intent.action.MAIN -e toasttext "
             "\"Boosting game %s\" -n bellavita.toast/.MainActivity",
             trim_newline(gamestart));
    system(command);
    performance_mode();
  } else if (mode == 2 && cur_mode != 2) {
    cur_mode = 2;
    printf("Applying powersave mode\n");
    powersave_mode();
  } else if (mode == 0 && cur_mode != 0) {
    cur_mode = 0;
    printf("Applying normal mode\n");
    normal_mode();
  }

  if (pid) {
    free(pid);
    pid = NULL;
  }
}

int main(void) {
  char *screenstate = NULL;
  char *low_power = NULL;

  perf_common();

  while (1) {
    snprintf(command, sizeof(command),
             "dumpsys window | grep -E 'mCurrentFocus|mFocusedApp' | grep -Eo "
             "\"$(cat /data/encore/gamelist.txt)\" | tail -n 1");
    gamestart = execute_command(command);

    snprintf(command, sizeof(command),
             "dumpsys display | grep \"mScreenState\" | awk -F'=' '{print $2}'");
    screenstate = execute_command(command);

    low_power = execute_command("settings get global low_power_sticky");

    if (gamestart && strcmp(trim_newline(screenstate), "ON") == 0) {
      // Apply performance mode
      apply_mode(1);
    } else if (low_power && strcmp(trim_newline(low_power), "1") == 0) {
      // Apply powersave mode
      apply_mode(2);
    } else {
      // Apply normal mode
      apply_mode(0);
    }

    if (gamestart) {
      printf("gamestart: %s\n", trim_newline(gamestart));
      free(gamestart);
      gamestart = NULL;
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
