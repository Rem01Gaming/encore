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

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>

#include "Encore.hpp"

namespace EncoreLog {

inline std::shared_ptr<spdlog::logger> g_logger;

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

inline std::shared_ptr<spdlog::logger> get() {
    if (!g_logger) init();
    return g_logger;
}

} // namespace EncoreLog

// Log with custom log tag
#define LOGT_TAG(TAG, ...) EncoreLog::get()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::trace, "{}: {}", TAG, fmt::format(__VA_ARGS__))
#define LOGD_TAG(TAG, ...) EncoreLog::get()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::debug, "{}: {}", TAG, fmt::format(__VA_ARGS__))
#define LOGI_TAG(TAG, ...) EncoreLog::get()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::info, "{}: {}", TAG, fmt::format(__VA_ARGS__))
#define LOGW_TAG(TAG, ...) EncoreLog::get()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::warn, "{}: {}", TAG, fmt::format(__VA_ARGS__))
#define LOGE_TAG(TAG, ...) EncoreLog::get()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::err, "{}: {}", TAG, fmt::format(__VA_ARGS__))
#define LOGC_TAG(TAG, ...) EncoreLog::get()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::critical, "{}: {}", TAG, fmt::format(__VA_ARGS__))

#define LOGT(...) LOGT_TAG(LOG_TAG, __VA_ARGS__)
#define LOGD(...) LOGD_TAG(LOG_TAG, __VA_ARGS__)
#define LOGI(...) LOGI_TAG(LOG_TAG, __VA_ARGS__)
#define LOGW(...) LOGW_TAG(LOG_TAG, __VA_ARGS__)
#define LOGE(...) LOGE_TAG(LOG_TAG, __VA_ARGS__)
#define LOGC(...) LOGC_TAG(LOG_TAG, __VA_ARGS__)