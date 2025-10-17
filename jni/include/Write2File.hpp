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

#include <fstream>
#include <sstream>

/**
* @brief Writes formatted content to a file, truncating it if it exists.
*
* This function formats a string using the provided arguments and writes it to the specified file.
* The file is always truncated before writing. No file locking is performed.
* @tparam Args The types of the arguments for formatting.
* @param filename The path to the file to write to.
* @param args The arguments to format into the string.
* @return true if the write operation was successful, false otherwise.
*/
template<typename... Args>
bool write2file(const std::string& filename, Args&&... args) {
   std::ostringstream oss;
   (oss << ... << std::forward<Args>(args));

   std::ofstream file(filename, std::ios::trunc);
   if (!file.is_open()) {
       return false;
   }

   file << oss.str();
   return file.good();
}
