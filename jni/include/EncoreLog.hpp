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

#include <memory>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "Encore.hpp"

namespace EncoreLog {

/// Global logger instance
inline std::shared_ptr<spdlog::logger> g_logger;

/**
 * @brief Initialize the logging system
 *
 * @param log_path Path to the log file (defaults to LOG_FILE)
 *
 * @details Creates a basic file logger with the pattern "YYYY-MM-DD HH:MM:SS.mmm L message"
 * @note If initialization fails, the program will exit with EXIT_FAILURE.
 */
inline void init(const std::string &log_path = LOG_FILE) {
    try {
        g_logger = spdlog::basic_logger_mt("Encore", log_path);
        g_logger->set_pattern("%Y-%m-%d %H:%M:%S.%e %L %v");
        g_logger->set_level(spdlog::level::trace);
        g_logger->flush_on(spdlog::level::info);
    } catch (const spdlog::spdlog_ex &ex) {
        fprintf(stderr, "EncoreLog init failed: %s\n", ex.what());
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Get the logger instance
 *
 * @return std::shared_ptr<spdlog::logger> Shared pointer to the logger
 *
 * @details If the logger hasn't been initialized yet, it will be initialized
 *          with default parameters before being returned.
 */
inline std::shared_ptr<spdlog::logger> get() {
    if (!g_logger) init();
    return g_logger;
}

/**
 * @brief Flush all pending log messages
 *
 * @details Forces immediate write of all buffered log messages to disk
 */
inline void flush() {
    if (g_logger) {
        g_logger->flush();
    }
}

} // namespace EncoreLog

#define LOGT_TAG(TAG, ...)                                                                                                       \
    EncoreLog::get()->log(                                                                                                       \
        spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::trace, "{}: {}", TAG, fmt::format(__VA_ARGS__)   \
    )

#define LOGD_TAG(TAG, ...)                                                                                                       \
    EncoreLog::get()->log(                                                                                                       \
        spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::debug, "{}: {}", TAG, fmt::format(__VA_ARGS__)   \
    )

#define LOGI_TAG(TAG, ...)                                                                                                       \
    EncoreLog::get()->log(                                                                                                       \
        spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::info, "{}: {}", TAG, fmt::format(__VA_ARGS__)    \
    )

#define LOGW_TAG(TAG, ...)                                                                                                       \
    EncoreLog::get()->log(                                                                                                       \
        spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::warn, "{}: {}", TAG, fmt::format(__VA_ARGS__)    \
    )

#define LOGE_TAG(TAG, ...)                                                                                                       \
    EncoreLog::get()->log(                                                                                                       \
        spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::err, "{}: {}", TAG, fmt::format(__VA_ARGS__)     \
    )

#define LOGC_TAG(TAG, ...)                                                                                                       \
    EncoreLog::get()->log(                                                                                                       \
        spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION},                                                                 \
        spdlog::level::critical,                                                                                                 \
        "{}: {}",                                                                                                                \
        TAG,                                                                                                                     \
        fmt::format(__VA_ARGS__)                                                                                                 \
    )

#define LOGT(...) LOGT_TAG(LOG_TAG, __VA_ARGS__)
#define LOGD(...) LOGD_TAG(LOG_TAG, __VA_ARGS__)
#define LOGI(...) LOGI_TAG(LOG_TAG, __VA_ARGS__)
#define LOGW(...) LOGW_TAG(LOG_TAG, __VA_ARGS__)
#define LOGE(...) LOGE_TAG(LOG_TAG, __VA_ARGS__)
#define LOGC(...) LOGC_TAG(LOG_TAG, __VA_ARGS__)

namespace EncoreLog {

/**
 * @brief Set the log level
 *
 * @param level Log level (0-5)
 *
 * @note If an invalid level is provided, it defaults to info.
 */
inline void set_log_level(int level) {
    static int prev_level = 3;
    spdlog::level::level_enum spdlog_level;

    if (prev_level == level) {
        return;
    }

    switch (level) {
        case 0: spdlog_level = spdlog::level::critical; break;
        case 1: spdlog_level = spdlog::level::err; break;
        case 2: spdlog_level = spdlog::level::warn; break;
        case 3: spdlog_level = spdlog::level::info; break;
        case 4: spdlog_level = spdlog::level::debug; break;
        case 5: spdlog_level = spdlog::level::trace; break;
        default: spdlog_level = spdlog::level::info; break;
    }

    prev_level = level;
    auto logger = get();
    logger->set_level(spdlog_level);
    LOGI_TAG("EncoreLog", "Log level changed to {}", spdlog::level::to_string_view(spdlog_level));
}

} // namespace EncoreLog
