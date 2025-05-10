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
 * Function Name      : log_encore
 * Inputs             : level - Log level
 *                      message (const char *) - message to log
 *                      variadic arguments - additional arguments for message
 * Returns            : None
 * Description        : print and logs a formatted message with a timestamp
 *                      to a log file.
 ***********************************************************************************/
void log_encore(LogLevel level, const char* message, ...) {
    const char* level_str[] = {"D", "I", "W", "E", "F"};

    char* timestamp = timern();
    char logMesg[MAX_OUTPUT_LENGTH];
    va_list args;
    va_start(args, message);
    vsnprintf(logMesg, sizeof(logMesg), message, args);
    va_end(args);

    write2file(LOG_FILE, true, true, "%s %s %s: %s\n", timestamp, level_str[level], LOG_TAG, logMesg);
}
