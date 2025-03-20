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
 * Inputs             : file_path (const char *) - path to the file
 *                      content (const char *) - content to write
 *                      mode (const char) - 1 for append and 0 for write
 * Returns            : char - 0 if write successful
 *                            -1 if file does not exist or inaccessible
 * Description        : Write the provided content to the specified file.
 ***********************************************************************************/
char write2file(const char* file_path, const char* content, const char mode) {
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
    if (file) [[clang::likely]] {
        fprintf(file, "%s\n", content);
        fclose(file);
        return 0;
    }

    return -1;
}

/***********************************************************************************
 * Function Name      : create_lock_file
 * Inputs             : None
 * Returns            : char - 0 if lock file created successfully
 *                            -1 if another instance running
 * Description        : Create lock file and check if there's any another instance of
 *                      this daemon running.
 ***********************************************************************************/
char create_lock_file(void) {
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
