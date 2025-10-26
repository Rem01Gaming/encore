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

#pragma once

#include <string>
#include <vector>

struct CliCommand {
    std::string name;
    std::string description;
    std::string usage;
    int min_args;
    int max_args;
    int (*handler)(const std::vector<std::string> &args);
};

void cli_usage(const char *program_name);
void cli_usage_command(const CliCommand &cmd);
int encore_cli(int argc, char *argv[]);
