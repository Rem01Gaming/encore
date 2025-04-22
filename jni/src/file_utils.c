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
 *                      content (const char *) - content to write
 *                      append (const bool) - true for append and false for write
 *                      use_flock (const bool) - true for acquire lock and false for no lock
 * Returns            : int - 0 if write successful
 *                           -1 if file does not exist or inaccessible
 * Description        : Write the provided content to the specified file.
 * Note               : Do not use flock on /sdcard due FUSE, write will fail.
 ***********************************************************************************/
int write2file(const char *filename, const char *data, const bool append, const bool use_flock) {
    // Reject empty data
    ssize_t bytes_to_write = strlen(data);
    if (!data || bytes_to_write == 0) {
        return -1;
    }

    int flags = O_WRONLY | O_CREAT;
    flags |= append ? O_APPEND : O_TRUNC;

    int fd = open(filename, flags, 0644);
    if (fd == -1)
        return -1;

    // Conditional locking
    if (use_flock && flock(fd, LOCK_EX) == -1) {
        close(fd);
        return -1;
    }

    // Write data to the file
    ssize_t written = write(fd, data, bytes_to_write);

    // Conditional unlock
    if (use_flock)
        flock(fd, LOCK_UN);

    close(fd);
    return (written == bytes_to_write) ? 0 : -1;
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
