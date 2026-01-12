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

#include <csignal>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <vector>

#include "EncoreLog.hpp"

namespace SignalHandler {

/// Global flag to prevent reentrant signal handling
static std::sig_atomic_t handling_signal = 0;

/**
 * @brief Get signal name from signal number
 */
inline std::string signal_name(int sig) {
    switch (sig) {
        case SIGSEGV: return "SIGSEGV (Segmentation Fault)";
        case SIGABRT: return "SIGABRT (Abort)";
        case SIGILL: return "SIGILL (Illegal Instruction)";
        case SIGFPE: return "SIGFPE (Floating Point Exception)";
        case SIGBUS: return "SIGBUS (Bus Error)";
        case SIGTERM: return "SIGTERM (Termination)";
        case SIGINT: return "SIGINT (Interrupt)";
        case SIGKILL: return "SIGKILL (Kill)";
        case SIGHUP: return "SIGHUP (Hangup)";
        case SIGQUIT: return "SIGQUIT (Quit)";
        case SIGTRAP: return "SIGTRAP (Trap)";
        default: return std::to_string(sig);
    }
}

/**
 * @brief Signal handler for critical signals
 */
inline void signal_handler(int sig) {
    if (handling_signal) {
        _exit(EXIT_FAILURE);
    }

    handling_signal = 1;
    LOGC_TAG("SignalHandler", "Received signal {}", signal_name(sig));

    EncoreLog::flush();
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

/**
 * @brief Set up signal handlers for common exit/crash scenarios
 */
inline void setup_signal_handlers() {
    std::signal(SIGSEGV, signal_handler); // Segfault
    std::signal(SIGABRT, signal_handler); // Abort
    std::signal(SIGILL, signal_handler);  // Illegal instruction
    std::signal(SIGFPE, signal_handler);  // Floating point exception
    std::signal(SIGBUS, signal_handler);  // Bus error
    std::signal(SIGTERM, signal_handler); // Terminated by user
    std::signal(SIGINT, signal_handler);  // Interupt
    std::signal(SIGHUP, signal_handler);  // Hang up
}

/**
 * @brief Clean up before exit
 */
inline void cleanup_before_exit() {
    // Flush logs
    EncoreLog::flush();
}

} // namespace SignalHandler
