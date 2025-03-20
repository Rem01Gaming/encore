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
 * Function Name      : ksu_grant_root
 * Inputs             : None
 * Returns            : bool - true if granted successfully
 *                             false if request denied or error
 * Description        : Request SU permission from KernelSU via prctl.
 ***********************************************************************************/
[[nodiscard]] bool ksu_grant_root(void) {
    uint32_t result = 0;
    prctl(0xdeadbeef, 0, 0, 0, &result);
    return result == 0xdeadbeef;
}
