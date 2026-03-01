/*
 * Copyright (C) 2024-2026 Rem01Gaming
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
#include <cstring>
#include <functional>
#include <string>
#include <unistd.h>
#include <vector>

#include "EncoreLog.hpp"

namespace SignalHandler {

/// Prevents re-entrant crash handling.
inline std::sig_atomic_t handling_signal = 0;

/// Callback type for controllable signals (SIGHUP, SIGUSR1, SIGUSR2).
using SignalCallback = std::function<void(int sig)>;

/// Registered callbacks for SIGHUP.
inline std::vector<SignalCallback> sighup_callbacks;
/// Registered callbacks for SIGUSR1.
inline std::vector<SignalCallback> sigusr1_callbacks;
/// Registered callbacks for SIGUSR2.
inline std::vector<SignalCallback> sigusr2_callbacks;

/**
 * @brief Register a callback to be invoked when SIGHUP is received.
 *
 * @param cb Callable with signature void(int sig).
 * @note Callbacks are invoked from the signal handler thread context.
 *       Keep them short and avoid heavy locking.
 */
inline void on_sighup(SignalCallback cb) {
    sighup_callbacks.push_back(std::move(cb));
}

/**
 * @brief Register a callback to be invoked when SIGUSR1 is received.
 *
 * @param cb Callable with signature void(int sig).
 */
inline void on_sigusr1(SignalCallback cb) {
    sigusr1_callbacks.push_back(std::move(cb));
}

/**
 * @brief Register a callback to be invoked when SIGUSR2 is received.
 *
 * @param cb Callable with signature void(int sig).
 */
inline void on_sigusr2(SignalCallback cb) {
    sigusr2_callbacks.push_back(std::move(cb));
}

// ---------------------------------------------------------------------------
// Async-signal-safe helpers
// ---------------------------------------------------------------------------

/**
 * @brief Write a null-terminated string to stderr without using stdio.
 */
inline void safe_write(const char *msg) {
    if (msg) write(STDERR_FILENO, msg, strlen(msg));
}

/**
 * @brief Write a short fatal log line to stderr that is async-signal-safe.
 */
inline void safe_log_signal(int sig) {
    const char *name = nullptr;
    switch (sig) {
        case SIGSEGV: name = "SIGSEGV (Segmentation Fault)"; break;
        case SIGABRT: name = "SIGABRT (Abort)"; break;
        case SIGILL: name = "SIGILL (Illegal Instruction)"; break;
        case SIGFPE: name = "SIGFPE (Floating Point Exception)"; break;
        case SIGBUS: name = "SIGBUS (Bus Error)"; break;
        case SIGTERM: name = "SIGTERM (Termination)"; break;
        case SIGINT: name = "SIGINT (Interrupt)"; break;
        case SIGQUIT: name = "SIGQUIT (Quit)"; break;
        case SIGTRAP: name = "SIGTRAP (Trap)"; break;
        case SIGHUP: name = "SIGHUP (Hangup)"; break;
        case SIGUSR1: name = "SIGUSR1"; break;
        case SIGUSR2: name = "SIGUSR2"; break;
        default: name = "(unknown signal)"; break;
    }
    safe_write("[SignalHandler] received signal: ");
    safe_write(name);
    safe_write("\n");
}

// ---------------------------------------------------------------------------
// Signal handlers
// ---------------------------------------------------------------------------

/**
 * @brief Handler for fatal/crash signals (SIGSEGV, SIGABRT, SIGILL, etc.).
 */
inline void crash_signal_handler(int sig) {
    if (handling_signal) {
        _exit(EXIT_FAILURE); // Re-entrant: bail out immediately
    }
    handling_signal = 1;

    safe_log_signal(sig);
    fsync(STDERR_FILENO);

    // Best-effort spdlog flush. This is NOT async-signal-safe, but we are
    // about to terminate anyway, the risk of deadlock is acceptable here
    // versus losing the last log lines entirely.
    EncoreLog::flush();

    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

/**
 * @brief Handler for graceful-exit signals (SIGTERM, SIGINT).
 */
inline void exit_signal_handler(int sig) {
    if (handling_signal) {
        _exit(EXIT_FAILURE);
    }
    handling_signal = 1;

    safe_log_signal(sig);
    fsync(STDERR_FILENO);
    EncoreLog::flush();

    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

/**
 * @brief Handler for controllable signals (SIGHUP, SIGUSR1, SIGUSR2).
 */
inline void user_signal_handler(int sig) {
    LOGI_TAG("SignalHandler", "Received signal {}", sig);

    const std::vector<SignalCallback> *cbs = nullptr;
    switch (sig) {
        case SIGHUP: cbs = &sighup_callbacks; break;
        case SIGUSR1: cbs = &sigusr1_callbacks; break;
        case SIGUSR2: cbs = &sigusr2_callbacks; break;
        default: return;
    }

    for (const auto &cb : *cbs) {
        if (cb) {
            try {
                cb(sig);
            } catch (const std::exception &e) {
                LOGE_TAG("SignalHandler", "Callback exception for signal {}: {}", sig, e.what());
            } catch (...) {
                LOGE_TAG("SignalHandler", "Unknown callback exception for signal {}", sig);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

/**
 * @brief Install signal handlers for all common exit, crash, and user signals.
 *
 * Call once at daemon startup, after logging is initialized.
 *
 * Crash signals  → crash_signal_handler
 * Exit signals   → exit_signal_handler
 * User signals   → user_signal_handler
 */
inline void setup_signal_handlers() {
    // Crash signals
    std::signal(SIGSEGV, crash_signal_handler);
    std::signal(SIGABRT, crash_signal_handler);
    std::signal(SIGILL, crash_signal_handler);
    std::signal(SIGFPE, crash_signal_handler);
    std::signal(SIGBUS, crash_signal_handler);

    // Graceful-exit signals
    std::signal(SIGTERM, exit_signal_handler);
    std::signal(SIGINT, exit_signal_handler);

    // Controllable / user signals
    std::signal(SIGHUP, user_signal_handler);
    std::signal(SIGUSR1, user_signal_handler);
    std::signal(SIGUSR2, user_signal_handler);
}

// ---------------------------------------------------------------------------
// Cleanup
// ---------------------------------------------------------------------------

/**
 * @brief Flush all pending log messages before exit.
 */
inline void cleanup_before_exit() {
    EncoreLog::flush();
}

} // namespace SignalHandler