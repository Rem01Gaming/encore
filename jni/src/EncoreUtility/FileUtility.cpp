/*
* Copyright (C) 2024-2026 Rem01Gaming
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "EncoreUtility.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/file.h>

bool create_lock_file(void) {
    int fd = open(LOCK_FILE, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        LOGC("Unable to open {}: {}", LOCK_FILE, strerror(errno));
        return false;
    }

    struct flock fl = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0,
        .l_pid = 0,
    };

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        close(fd);
        return false;
    }

    // Note: The file descriptor must remain open to maintain the lock
    return true;
}

bool check_java_daemon(void) {
    int fd = open(JAVA_LOCK_FILE, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        LOGC("Unable to open {}: {}", JAVA_LOCK_FILE, strerror(errno));
        return false;
    }

    struct flock fl = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0,
        .l_pid = 0,
    };

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        // Java holds the lock
        close(fd);
        return true;
    }

    // Java is not running, release immediately and close
    fl.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &fl);
    close(fd);
    return false;
}
