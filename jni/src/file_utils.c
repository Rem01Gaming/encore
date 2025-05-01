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
 * Function Name      : write2file
 * Inputs             : filename (const char *) - path to the file
 *                      append (const bool) - true for append and false for write
 *                      use_flock (const bool) - true for acquire lock and false for no lock
 *                      data (const char *) - format string for content
 * Returns            : int - 0 if write successful
 *                           -1 for any error
 * Description        : Writes formatted content to the specified file.
 * Note               : Do not use flock on /sdcard (FUSE limitation)
 ***********************************************************************************/
int write2file(const char* filename, const bool append, const bool use_flock, const char* data, ...) {
    // Validate format string
    if (!data)
        return -1;

    // Format variable arguments
    char content[MAX_DATA_LENGTH];
    va_list args;
    va_start(args, data);
    int len = vsnprintf(content, sizeof(content), data, args);
    va_end(args);

    // Empty content or Invalid format
    if (len <= 0)
        return -1;

    // Truncation occurred (buffer limits)
    if (len >= (int)sizeof(content))
        return -1;

    // Open file with appropriate mode
    int flags = O_WRONLY | O_CREAT;
    flags |= append ? O_APPEND : O_TRUNC;
    int fd = open(filename, flags, 0644);
    if (fd == -1)
        return -1;

    // Apply file lock if requested
    if (use_flock && flock(fd, LOCK_EX) == -1) {
        close(fd);
        return -1;
    }

    // Write formatted content
    ssize_t written = write(fd, content, len);

    // Cleanup resources
    if (use_flock)
        flock(fd, LOCK_UN);
    close(fd);

    // Verify full content was written
    return (written == len) ? 0 : -1;
}

/***********************************************************************************
 * Function Name      : create_lock_file
 * Inputs             : None
 * Returns            : int - 0 if lock file created successfully
 *                           -1 if another instance running
 * Description        : Create lock file and check if there's any another instance of
 *                      this daemon running.
 ***********************************************************************************/
int create_lock_file(void) {
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
