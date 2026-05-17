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

#include "ProcStat.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>

namespace UtilityAwareScheduler::internal {

std::vector<CpuTicks> read_cpu_ticks() {
    std::ifstream file("/proc/stat");
    std::vector<CpuTicks> result;
    std::string line;

    while (std::getline(file, line)) {
        if (line.size() < 4) continue;
        if (line[0] != 'c' || line[1] != 'p' || line[2] != 'u') continue;
        if (!std::isdigit(static_cast<unsigned char>(line[3]))) continue;

        std::istringstream ss(line);
        std::string label;
        uint64_t user = 0, nice = 0, system = 0, idle = 0;
        uint64_t iowait = 0, irq = 0, softirq = 0, steal = 0;

        ss >> label >> user >> nice >> system >> idle >> iowait >> irq >> softirq;
        ss >> steal;

        if (ss.fail() && ss.eof() && steal == 0) {
            ss.clear();
        }

        int index = std::stoi(label.substr(3));

        if (index >= static_cast<int>(result.size())) {
            result.resize(index + 1);
        }

        CpuTicks ticks;
        ticks.busy = user + nice + system + irq + softirq;
        ticks.total = ticks.busy + idle + iowait + steal;
        result[index] = ticks;
    }

    return result;
}

} // namespace UtilityAwareScheduler::internal
