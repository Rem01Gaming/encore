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

#include "EncoreConfig.hpp"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/error/en.h>

bool load_gamelist_from_json(const std::string &filename, GameRegistry &registry) {
    FILE *fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        LOGE_TAG("JsonParser", "{}: {}", filename, strerror(errno));
        return false;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document doc;
    doc.ParseStream(is);

    fclose(fp);

    if (doc.HasParseError()) {
        LOGE_TAG(
            "JsonParser", "gamelist.json parse error: {} (Offset: {})",
            rapidjson::GetParseError_En(doc.GetParseError()), doc.GetErrorOffset());
        return false;
    }

    if (!doc.IsArray()) {
        LOGE_TAG("JsonParser", "gamelist.json root is not an array");
        return false;
    }

    std::vector<EncoreGameList> new_list;

    for (rapidjson::SizeType i = 0; i < doc.Size(); i++) {
        const rapidjson::Value &item = doc[i];

        if (!item.IsObject()) {
            LOGE_TAG("JsonParser", "Item {} is not an object, skipping", i);
            continue;
        }

        if (!item.HasMember("package_name") || !item["package_name"].IsString()) {
            LOGE_TAG("JsonParser", "Missing or invalid package_name for item {}, skipping", i);
            continue;
        }

        EncoreGameList game;
        game.package_name = item["package_name"].GetString();

        if (item.HasMember("lite_mode") && item["lite_mode"].IsBool()) {
            game.lite_mode = item["lite_mode"].GetBool();
        } else {
            game.lite_mode = false;
        }

        if (item.HasMember("enable_dnd") && item["enable_dnd"].IsBool()) {
            game.enable_dnd = item["enable_dnd"].GetBool();
        } else {
            game.enable_dnd = false;
        }

        new_list.push_back(game);
    }

    registry.update_gamelist(new_list);
    return true;
}
