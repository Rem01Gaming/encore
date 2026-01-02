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

#include "EncoreUtility.hpp"

#include <sys/file.h>

bool create_lock_file(void) {
    int fd = open(LOCK_FILE, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        perror("open");
        return false;
    }

    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
        close(fd);
        return false;
    }

    return true;
}

bool check_dumpsys_sanity(void) {
    FILE* file = fopen("/system/bin/dumpsys", "rb");
    int ch;
    if (!file) {
        fprintf(stderr, "/system/bin/dumpsys: %s\n", strerror(errno));
        LOGC("/system/bin/dumpsys: {}", strerror(errno));
        goto insane;
    }

    ch = fgetc(file);
    if (ch == EOF) {
        if (feof(file)) {
            fprintf(stderr, "/system/bin/dumpsys was tampered by kill logger module\n");
            LOGC("/system/bin/dumpsys was tampered by kill logger module");
            goto insane;
        }

        fprintf(stderr, "/system/bin/dumpsys: %s\n", strerror(errno));
        LOGC("/system/bin/dumpsys: {}", strerror(errno));
        goto insane;
    }

    fclose(file);
    return true;

insane:
    fclose(file);
    return false;
}
