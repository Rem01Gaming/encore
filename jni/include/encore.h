#ifndef ENCORE_H
#define ENCORE_H

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define LOOP_INTERVAL 15
#define MAX_DATA_LENGTH 1024
#define MAX_COMMAND_LENGTH 600
#define MAX_OUTPUT_LENGTH 256
#define MAX_PATH_LENGTH 256

#define NOTIFY_TITLE "Encore Tweaks"
#define LOG_TAG "EncoreTweaks"

#define LOCK_FILE "/dev/encore_lockfile"
#define LOG_FILE "/dev/encore_log"
#define PROFILE_MODE "/dev/encore_mode"
#define GAME_INFO "/dev/encore_game_info"
#define GAMELIST "/dev/encore_gamelist"
#define MODULE_PROP "/data/adb/modules/encore/module.prop"
#define MODULE_UPDATE "/data/adb/modules/encore/update"

#define MY_PATH                                                                                                                    \
    "PATH=/system/bin:/system/xbin:/data/adb/ap/bin:/data/adb/ksu/bin:/data/adb/magisk:/debug_ramdisk:/sbin:/sbin/su:/su/bin:/su/" \
    "xbin:/data/data/com.termux/files/usr/bin"

#define IS_MLBB(gamestart)                                                                               \
    (strcmp(gamestart, "com.mobile.legends") == 0 || strcmp(gamestart, "com.mobilelegends.hwag") == 0 || \
     strcmp(gamestart, "com.mobiin.gp") == 0 || strcmp(gamestart, "com.mobilechess.gp") == 0)

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
    MLBB_NOT_RUNNING,
    MLBB_RUN_BG,
    MLBB_RUNNING
} MLBBState;

extern char* gamestart;
extern pid_t game_pid;

/*
 * If you're here for function comments, you
 * are in the wrong place.
 */

// Misc Utilities
void sighandler(const int signal);
char* trim_newline(char* string);
void notify(const char* message);
void is_kanged(void);
char* timern(void);
bool return_true(void);
bool return_false(void);

// Shell and Command execution
char* execute_command(const char* format, ...);
char* execute_direct(const char* path, const char* arg0, ...);
int systemv(const char* format, ...);

// File Utilities
int create_lock_file(void);
int write2file(const char* filename, const bool append, const bool use_flock, const char* data, ...);

// Logging system
void log_encore(LogLevel level, const char* message, ...);

// KernelSU Utilities
bool ksu_grant_root(void);

// Process Utilities
void set_priority(const pid_t pid);
pid_t pidof(const char* name);
int uidof(pid_t pid);

// MLBB Handler
extern pid_t mlbb_pid;
MLBBState handle_mlbb(const char* gamestart);

// Encore Profiler
extern bool (*get_screenstate)(void);
extern bool (*get_low_power_state)(void);
char* get_gamestart(void);
bool get_screenstate_normal(void);
bool get_low_power_state_normal(void);
void run_profiler(const int profile);

#endif // ENCORE_H
