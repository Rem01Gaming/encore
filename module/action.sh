#
# Copyright (C) 2024-2025 Rem01Gaming
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

if [ -n "$MMRL" ]; then
	echo "- This action script is NOT intended to run on MMRL..."
	echo "- Please open Encore Tweaks WebUI by clicking the module card."
	exit 0
fi

if [ -n "$MAGISKTMP" ]; then
	pm path io.github.a13e300.ksuwebui >/dev/null 2>&1 && {
		echo "- Launching WebUI in KSUWebUIStandalone..."
		am start -n "io.github.a13e300.ksuwebui/.WebUIActivity" -e id "encore"
		exit 0
	}
	pm path com.dergoogler.mmrl >/dev/null 2>&1 && {
		echo "- Launching WebUI in MMRL WebUI..."
		am start -n "com.dergoogler.mmrl/.ui.activity.webui.WebUIActivity" -e MOD_ID "encore"
		exit 0
	}
	pm path com.dergoogler.mmrl.webuix >/dev/null 2>&1 && {
		echo "- Launching WebUI in WebUI X..."
		am start -n "com.dergoogler.mmrl.webuix/.ui.activity.webui.WebUIActivity" -e MOD_ID "encore"
		exit 0
	}
fi

echo "! Install KsuWebUI for WebUI access"
sleep 2
am start -a android.intent.action.VIEW -d https://github.com/5ec1cff/KsuWebUIStandalone/releases
exit 0
