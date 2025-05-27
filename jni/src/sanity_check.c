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
 * Function Name      : sanity_check
 * Inputs             : None
 * Returns            : int - 0 if check successfull
 *                           -1 if check fail
 * Description        : Sanity check, capturing bugs before the module
 *                      installed/running.
 ***********************************************************************************/
int sanity_check(void) {
    char check_fail = 0;
    char* gamestart_test = execute_command("dumpsys window visible-apps | grep 'package=.* '");

    if (!gamestart_test) {
        gamestart_test = execute_command("dumpsys activity recents | grep 'Recent #'");
        get_gamestart = get_gamestart_method2;
    }

    if (!gamestart_test) {
        fprintf(stderr, "SANITY CHECK: Unable to fetch focused nor recent app!\n");
        check_fail = 1;
    }

    if (check_fail)
        return -1;

    return 0;
}
