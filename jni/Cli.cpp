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

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "Cli.hpp"

#include <Encore.hpp>
#include <GameRegistry.hpp>

std::string get_module_version() {
    std::ifstream prop_file(MODULE_PROP);
    std::string line;
    std::string version = "unknown";

    if (!prop_file.is_open()) {
        fprintf(stderr, "\033[33mERROR:\033[0m Could not open %s\n", MODULE_PROP);
        return version;
    }

    while (std::getline(prop_file, line)) {
        if (line.find("version=") == 0) {
            // Remove "version=" prefix
            version = line.substr(8);

            // Remove quotes if present
            if (!version.empty() && version.front() == '"' && version.back() == '"') {
                version = version.substr(1, version.length() - 2);
            }

            break;
        }
    }

    prop_file.close();
    return version;
}

int version_handler(const std::vector<std::string> &args) {
    (void)args;

    std::string module_version = get_module_version();
    printf("Encore Tweaks %s\n", module_version.c_str());
    printf("Built on %s %s\n", __TIME__, __DATE__);
    return EXIT_SUCCESS;
}

int setup_gamelist_handler(const std::vector<std::string> &args) {
    if (access(ENCORE_GAMELIST, F_OK) == 0) {
        fprintf(stderr, "\033[31mERROR:\033[0m %s already exists!\n", ENCORE_GAMELIST);
        return EXIT_FAILURE;
    }

    bool success = GameRegistry::populate_from_base(ENCORE_GAMELIST, args[0]);
    if (!success) {
        fprintf(
            stderr, "\033[31mERROR:\033[0m Failed to setup gamelist from %s\n", args[0].c_str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int check_gamelist_handler(const std::vector<std::string> &args) {
    (void)args;

    if (access(ENCORE_GAMELIST, F_OK) != 0) {
        printf("\033[33mERROR:\033[0m %s does not exist\n", ENCORE_GAMELIST);
        return EXIT_FAILURE;
    }

    GameRegistry registry;
    if (!registry.load_from_json(ENCORE_GAMELIST)) {
        fprintf(stderr, "\033[31mERROR:\033[0m Failed to parse %s\n", ENCORE_GAMELIST);
        return EXIT_FAILURE;
    }

    printf("%s is valid\n", ENCORE_GAMELIST);
    printf("Registered games: %zu\n", registry.size());
    return EXIT_SUCCESS;
}

// Command definitions
std::vector<CliCommand> commands = {
    {"setup_gamelist", "Set up initial gamelist from base file", "setup_gamelist <base_file_path>",
     1, 1, setup_gamelist_handler},
    {"check_gamelist", "Validate gamelist file", "check_gamelist", 0, 0, check_gamelist_handler},
    {"version", "Show version information", "version", 0, 0, version_handler}};

void cli_usage(const char *program_name) {
    printf("Encore Tweaks CLI\n\n");
    printf("Usage: %s <COMMAND> [OPTIONS]\n\n", program_name);
    printf("Commands:\n");

    for (const auto &cmd : commands) {
        printf("  %-20s %s\n", cmd.name.c_str(), cmd.description.c_str());
    }

    printf("\nOptions:\n");
    printf("  -h, --help          Show this help message\n");
    printf("  -V, --version       Show version information\n");
    printf("\nRun '%s <COMMAND> --help' for more information on a command.\n", program_name);
}

void cli_usage_command(const CliCommand &cmd) {
    printf("Usage: encored %s\n\n", cmd.usage.c_str());
    printf("%s\n", cmd.description.c_str());
}

int encore_cli(int argc, char *argv[]) {
    const char *program_name = argv[0];

    if (argc == 1) {
        cli_usage(program_name);
        return EXIT_FAILURE;
    }

    std::string command = argv[1];
    std::vector<std::string> args;

    for (int i = 2; i < argc; i++) {
        args.push_back(argv[i]);
    }

    if (command == "-h" || command == "--help") {
        cli_usage(program_name);
        return EXIT_SUCCESS;
    }

    if (command == "-V" || command == "--version") {
        return version_handler({});
    }

    if (args.size() == 1 && (args[0] == "-h" || args[0] == "--help")) {
        for (const auto &cmd : commands) {
            if (cmd.name != command) continue;
            cli_usage_command(cmd);
            return EXIT_SUCCESS;
        }

        fprintf(stderr, "\033[31mERROR:\033[0m Unknown command: %s\n", command.c_str());
        return EXIT_FAILURE;
    }

    for (const auto &cmd : commands) {
        if (cmd.name == command) {
            size_t min_args = static_cast<size_t>(cmd.min_args);
            size_t max_args = static_cast<size_t>(cmd.max_args);

            if (args.size() < min_args || args.size() > max_args) {
                fprintf(
                    stderr, "\033[31mERROR:\033[0m Invalid number of arguments for '%s'\n",
                    command.c_str());
                cli_usage_command(cmd);
                return EXIT_FAILURE;
            }

            return cmd.handler(args);
        }
    }

    fprintf(stderr, "\033[31mERROR:\033[0m Unknown command: %s\n", command.c_str());
    fprintf(stderr, "See '%s --help' for available commands.\n", program_name);
    return EXIT_FAILURE;
}
