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
 * Function Name      : trim_newline
 * Inputs             : str (char *) - string to trim newline from
 * Returns            : char * - string without newline
 * Description        : Trims a newline character at the end of a string if
 *                      present.
 ***********************************************************************************/
[[gnu::always_inline]] char* trim_newline(char* string) {
    if (string == NULL)
        return NULL;

    char* end;
    if ((end = strchr(string, '\n')) != NULL)
        *end = '\0';

    return string;
}

/***********************************************************************************
 * Function Name      : notify
 * Inputs             : message (char *) - Message to display
 * Returns            : None
 * Description        : Push a notification.
 ***********************************************************************************/
void notify(const char* message) {
    int exit = systemv("su -lp 2000 -c \"/system/bin/cmd notification post -t '%s' '%s' '%s'\" >/dev/null", NOTIFY_TITLE, LOG_TAG, message);

    if (exit != 0) [[clang::unlikely]] {
        log_encore(LOG_ERROR, "Unable to post push notification, message: %s", message);
    }
}

/***********************************************************************************
 * Function Name      : timern
 * Inputs             : None
 * Returns            : char * - pointer to a statically allocated string
 *                      with the formatted time.
 * Description        : Generates a timestamp with the format
 *                      [YYYY-MM-DD HH:MM:SS.milliseconds].
 ***********************************************************************************/
char* timern(void) {
    static char timestamp[64];
    struct timeval tv;
    time_t current_time;
    struct tm* local_time;

    gettimeofday(&tv, NULL);
    current_time = tv.tv_sec;
    local_time = localtime(&current_time);

    if (local_time == NULL) [[clang::unlikely]] {
        strcpy(timestamp, "[TimeError]");
        return timestamp;
    }

    size_t format_result = strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local_time);
    if (format_result == 0) [[clang::unlikely]] {
        strcpy(timestamp, "[TimeFormatError]");
        return timestamp;
    }

    // Append milliseconds
    snprintf(timestamp + strlen(timestamp), sizeof(timestamp) - strlen(timestamp), ".%03ld", tv.tv_usec / 1000);
    
    return timestamp;
}

/***********************************************************************************
 * Function Name      : sighandler
 * Inputs             : int signal - exit signal
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
 * Function Name      : is_kanged
 * Inputs             : None
 * Returns            : None
 * Description        : Checks if the module renamed/modified by 3rd party.
 ***********************************************************************************/
void is_kanged(void) {
    if (systemv("grep -q '^name=Encore Tweaks$' %s", MODULE_PROP) != 0) [[clang::unlikely]] {
        goto doorprize;
    }

    if (systemv("grep -q '^author=Rem01Gaming$' %s", MODULE_PROP) != 0) [[clang::unlikely]] {
        goto doorprize;
    }

    return;

doorprize:
    log_encore(LOG_FATAL, "Module modified by 3rd party, exiting.");
    notify("Trying to rename me?");
    exit(EXIT_FAILURE);
}

/***********************************************************************************
 * Function Name      : return_true
 * Inputs             : None
 * Returns            : bool - only true
 * Description        : Will be used for error fallback.
 * Note               : Never call this function.
 ***********************************************************************************/
bool return_true(void) {
    return true;
}

/***********************************************************************************
 * Function Name      : return_false
 * Inputs             : None
 * Returns            : bool - only false
 * Description        : Will be used for error fallback.
 * Note               : Never call this function.
 ***********************************************************************************/
bool return_false(void) {
    return false;
}
