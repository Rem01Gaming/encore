#!/system/bin/sh
#
# Copyright (C) 2024-2026 Rem01Gaming
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# shellcheck disable=SC2016

MODULE_DIR="/data/adb/modules/encore"
THIS_SCRIPT="/data/adb/service.d/.encore_cleanup.sh"

if [ ! -d "$MODULE_DIR/disable" ]; then
  cat "$MODULE_DIR/module.prop.orig" >"$MODULE_DIR/module.prop"
  rm -f "$THIS_SCRIPT"
fi
