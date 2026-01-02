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
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "EncoreCLI.hpp"

#include <Encore.hpp>
#include <GameRegistry.hpp>

std::string get_module_version() {
    std::ifstream prop_file(MODULE_PROP);
    std::string line;
    std::string version = "unknown";

    if (!prop_file.is_open()) {
        std::cerr << "\033[33mERROR:\033[0m Could not open " << MODULE_PROP << std::endl;
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
    std::cout << "Encore Tweaks " << module_version << std::endl;
    std::cout << "Built on " << __TIME__ << " " << __DATE__ << std::endl;
    return EXIT_SUCCESS;
}

int setup_gamelist_handler(const std::vector<std::string> &args) {
    bool success = GameRegistry::populate_from_base(ENCORE_GAMELIST, args[0]);
    if (!success) {
        std::cerr << "\033[31mERROR:\033[0m Failed to setup gamelist from " << args[0] << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int check_gamelist_handler(const std::vector<std::string> &args) {
    (void)args;

    if (access(ENCORE_GAMELIST, F_OK) != 0) {
        std::cout << "\033[33mERROR:\033[0m " << ENCORE_GAMELIST << " does not exist" << std::endl;
        return EXIT_FAILURE;
    }

    GameRegistry registry;
    if (!registry.load_from_json(ENCORE_GAMELIST)) {
        std::cerr << "\033[31mERROR:\033[0m Failed to parse " << ENCORE_GAMELIST << std::endl;
        return EXIT_FAILURE;
    }

    std::cerr << ENCORE_GAMELIST << " is valid" << std::endl;
    std::cerr << "Registered games: " << registry.size() << std::endl;
    return EXIT_SUCCESS;
}

int daemon_handler(const std::vector<std::string> &args) {
    (void)args;

    // This will be implemented in Main.cpp
    extern int run_daemon();
    return run_daemon();
}

// clang-format off
std::vector<CliCommand> commands = {
    {
        "daemon",
        "Start Encore Tweaks daemon",
        "daemon",
        0,
        0,
        daemon_handler
    },
    {
        "setup_gamelist",
        "Setup initial gamelist from base file",
        "setup_gamelist <base_file_path>",
        1,
        1,
        setup_gamelist_handler
    },
    {
        "check_gamelist",
        "Validate gamelist file",
        "check_gamelist",
        0,
        0,
        check_gamelist_handler
    },
    {
        "version",
        "Show version information",
        "version",
        0,
        0,
        version_handler
    },
};
// clang-format on

void cli_usage(const char *program_name) {
    std::cout << "Encore Tweaks CLI" << std::endl << std::endl;
    std::cout << "Usage: " << program_name << " <COMMAND>" << std::endl << std::endl;
    std::cout << "Commands:" << std::endl;

    for (const auto &cmd : commands) {
        printf("  %-20s %s\n", cmd.name.c_str(), cmd.description.c_str());
    }

    std::cout << std::endl << "Options:" << std::endl;
    std::cout << "  -h, --help          Show this help message" << std::endl;
    std::cout << "  -V, --version       Show version information" << std::endl;
    std::cout << std::endl << "Run '" << program_name << " <COMMAND> --help' for more information on a command." << std::endl;
}

void cli_usage_command(const CliCommand &cmd) {
    std::cout << "Usage: encored " << cmd.usage << std::endl << std::endl;
    std::cout << cmd.description << std::endl;
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

        std::cerr << "\033[31mERROR:\033[0m Unknown command: " << command << std::endl;
        return EXIT_FAILURE;
    }

    for (const auto &cmd : commands) {
        if (cmd.name == command) {
            size_t min_args = static_cast<size_t>(cmd.min_args);
            size_t max_args = static_cast<size_t>(cmd.max_args);

            if (args.size() < min_args || args.size() > max_args) {
                std::cerr << "\033[31mERROR:\033[0m Invalid number of arguments for '" << command << "'" << std::endl;
                cli_usage_command(cmd);
                return EXIT_FAILURE;
            }

            return cmd.handler(args);
        }
    }

    std::cerr << "\033[31mERROR:\033[0m Unknown command: " << command << std::endl;
    std::cerr << "See '" << program_name << " --help' for available commands." << std::endl;
    return EXIT_FAILURE;
}
