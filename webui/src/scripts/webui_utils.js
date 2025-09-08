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

import { exec, toast } from 'kernelsu';
import encoreHappy from '/encore_happy.avif';
import encoreSleeping from '/encore_sleeping.avif';

const configPath = '/data/adb/.config/encore';
const modPath = '/data/adb/modules/encore';
const binPath = '/data/adb/modules/encore/system/bin';
const officialWebsite = 'https://encore.rem01gaming.dev/';
const donateUrl = 'https://t.me/rem01schannel/670';

// WebUI X API
const moduleInterface = window.$encore;
const fileInterface = window.$EnFile;

/* ======================== UTILITIES ======================== */
const runCommand = async (cmd, cwd = null) => {
  const { errno, stdout, stderr } = await exec(cmd, cwd ? { cwd } : {});
  return errno === 0 ? stdout.trim() : { error: stderr };
};

const showCustomModal = (title, msg) => {
  document.getElementById('custom_modal_title').textContent = title;
  document.getElementById('custom_modal_desc').textContent = msg;
  disableBodyScroll();
  custom_modal.showModal();
};

const saveLog = async () => {
  const output = await runCommand(`${binPath}/encore_utility save_logs`);
  if (output.error) {
    const save_log_fail = getTranslation("modal.save_log_fail");
    showCustomModal(save_log_fail, output.error);
    return;
  }

  const log_saved_title = getTranslation("modal.log_saved_title");
  const log_saved_desc = getTranslation("modal.log_saved_desc", output);
  showCustomModal(log_saved_title, log_saved_desc);
};

const openWebsite = async (link) => {
  await runCommand(`am start -a android.intent.action.VIEW -d ${link}`);
};

const createShortcut = async () => {
  if (moduleInterface.hasShortcut()) {
    const has_shortcut = getTranslation("toast.has_shortcut");
    toast(has_shortcut);
    return;
  }

  moduleInterface.createShortcut();
};

const setupShortcut = async () => {
  if (moduleInterface) {
    document.getElementById('create_shortcut_btn').classList.remove("hidden");
  }
}

const openInfoModal = (button) => {
  const titleKey = button.getAttribute('data-title-key');
  const descKey = button.getAttribute('data-desc-key');
  
  // Update modal content
  document.getElementById('info_modal_title').textContent = getTranslation(titleKey);
  document.getElementById('info_modal_desc').textContent = getTranslation(descKey);
  
  // Show modal
  disableBodyScroll();
  info_modal.showModal();
};

const disableBodyScroll = () => {
  document.documentElement.classList.add('modal-open');
};

const enableBodyScroll = () => {
  document.documentElement.classList.remove('modal-open');
};

/* ======================== SYSTEM INFO ======================== */
const getModuleVersion = async () => {
  let version = null;

  if (fileInterface) {
    const propPath = `${modPath}/module.prop`;
    if (fileInterface.exists(propPath)) {
      const content = fileInterface.read(propPath).trim();
      const match = content.match(/^version=(.*)$/m);
      if (match) {
        version = match[1].trim();
      }
    }
  } else {
    version = await runCommand(`[ -f module.prop ] && awk -F'=' '/version=/ {print $2}' module.prop || echo null`, modPath);
    version = version === 'null' ? null : version.trim();
  }

  if (version === 'null') {
    const unauthorized_mod_title = getTranslation("modal.unauthorized_mod_title");
    const unauthorized_mod_desc = getTranslation("modal.unauthorized_mod_desc");

    showCustomModal(unauthorized_mod_title, unauthorized_mod_desc);
    openWebsite(officialWebsite);
    return;
  }

  document.getElementById('module_version').textContent = version;
};

const getCurrentProfile = async () => {
  let profile = "Unknown";
  let output;

  if (fileInterface) {
    output = fileInterface.read(`${configPath}/current_profile`).trim();
  } else {
    output = await runCommand(`cat ${configPath}/current_profile`);
  }

  switch(output) {
    case "0":
      profile = "Initializing";
      break;
    case "1":
      profile = "Performance";
      break;
    case "2":
      profile = "Normal";
      break;
    case "3":
      profile = "Powersave";
      break;
    default:
      profile = "Unknown";
      break;
    }
    
  document.getElementById('encore_profile').textContent = profile;
};

const getChipset = async () => {
  const chipset = await runCommand(`getprop ro.board.platform`);
  let brand = "Unknown";
  let soc;

  if (fileInterface) {
    soc = fileInterface.read(`${configPath}/soc_recognition`).trim();
  } else {
    soc = await runCommand(`cat ${configPath}/soc_recognition`);
  }
  
  switch(soc) {
    case "1":
      brand = "MediaTek";
      break;
    case "2":
      brand = "Snapdragon";
      break;
    case "3":
      brand = "Exynos";
      break;
    case "4":
      brand = "Unisoc";
      break;
    case "5":
      brand = "Tensor";
      break;
    case "6":
      brand = "Intel";
      break;
    case "7":
      brand = "Tegra";
      break;
    case "8":
      brand = "Kirin";
      break;
    default:
      brand = "Unknown";
      break;
    }
    
  document.getElementById('chipset_name').textContent = `${brand} ${chipset}`;
};

const getKernelVersion = async () => {
  document.getElementById('kernel_version').textContent = await runCommand(`uname -r -m`);
};

const getAndroidSDK = async () => {
  let android_sdk;

  if (moduleInterface) {
    android_sdk = moduleInterface.getSdk();
  } else {
    android_sdk = await runCommand(`getprop ro.build.version.sdk`);
  }

  document.getElementById('android_sdk').textContent = android_sdk;
};

/* ======================== SERVICE MANAGEMENT ======================== */
const getServiceState = async () => {
  const status = document.getElementById('daemon_status');
  const image = document.getElementById('encore_logo');
  const pidElement = document.getElementById('daemon_pid');

  const pid = await runCommand('/system/bin/toybox pidof encored || echo null');
  pidElement.textContent = `Daemon PID: ${pid}`;

  if (pid === "null") {
    status.textContent = "Stopped 💤";
    image.src = encoreSleeping;
    return;
  }

  status.textContent = "Working ✨";
  image.src = encoreHappy;
};

/* ======================== CONFIGURATION ======================== */
const toggleConfig = async (file, isChecked, requireReboot = false) => {
  const value = isChecked ? '1' : '0';
  if (fileInterface) {
    fileInterface.write(`${configPath}/${file}`, value);
  } else {
    await runCommand(`echo ${value} >${configPath}/${file}`);
  }

  if (requireReboot) {
    const reboot_to_apply = getTranslation("toast.reboot_to_apply");
    toast(reboot_to_apply);
  }
};

const setupSwitch = async (id, file, rebootMessage = false) => {
  const switchElement = document.getElementById(id);
  let value;

  if (fileInterface) {
    value = fileInterface.read(`${configPath}/${file}`).trim();
  } else {
    value = await runCommand(`cat ${configPath}/${file}`);
  }

  switchElement.checked = value === '1';
  switchElement.addEventListener('change', () => toggleConfig(file, switchElement.checked, rebootMessage));
};

// Initialize switches
setupSwitch('lite_mode_switch', 'lite_mode');
setupSwitch('dnd_switch', 'dnd_gameplay');
setupSwitch('device_mitigation_switch', 'device_mitigation');

/* ======================== CPU GOVERNOR MANAGEMENT ======================== */
const changeCPUGovernor = async (governor, config) => {
  if (fileInterface) {
    fileInterface.write(`${configPath}/${config}`, governor);
  } else {
    await runCommand(`echo ${governor} >${configPath}/${config}`);
  }

  if (config === "powersave_cpu_gov") {
    await runCommand(`[ "$(<${configPath}/current_profile)" -eq 3 ] && encore_utility change_cpu_gov ${governor}`);
  } else if (config === "custom_default_cpu_gov") {
    await runCommand(`[ "$(<${configPath}/current_profile)" -eq 2 ] && encore_utility change_cpu_gov ${governor}`);
  }
};

const fetchCPUGovernors = async () => {
  let governors;
  if (fileInterface) {
    governors = fileInterface.read('/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors').trim().split(/\s+/);
  } else {
    governors = (await runCommand('cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors')).split(/\s+/);
  }

  ['default_cpu_gov', 'powersave_cpu_gov'].forEach(id => {
    const select = document.getElementById(id);
    select.innerHTML = governors.map(gov => `<option value="${gov}">${gov}</option>`).join('');
    select.addEventListener('change', () => changeCPUGovernor(select.value, id.includes('powersave') ? "powersave_cpu_gov" : "custom_default_cpu_gov"));
  });

  let default_cpu_gov;
  let powersave_cpu_gov;

  if (fileInterface) {
    if (fileInterface.exists(`${configPath}/custom_default_cpu_gov`)) {
      default_cpu_gov = fileInterface.read(`${configPath}/custom_default_cpu_gov`).trim();
    } else {
      default_cpu_gov = fileInterface.read(`${configPath}/default_cpu_gov`).trim();
    }

    powersave_cpu_gov = fileInterface.read(`${configPath}/powersave_cpu_gov`).trim();
  } else {
    default_cpu_gov = await runCommand(`[ -f ${configPath}/custom_default_cpu_gov ] && cat ${configPath}/custom_default_cpu_gov || cat ${configPath}/default_cpu_gov`);
    powersave_cpu_gov = await runCommand(`cat ${configPath}/powersave_cpu_gov`);
  }

  document.getElementById('default_cpu_gov').value = default_cpu_gov;
  document.getElementById('powersave_cpu_gov').value = powersave_cpu_gov;
};

/* ======================== GAMELIST MANAGEMENT ======================== */
const fetchGamelist = async () => {
  const input = document.getElementById('gamelist_textarea');
  let output;

  if (fileInterface) {
    output = fileInterface.read(`${configPath}/gamelist.txt`).trim();
  } else {
    output = await runCommand(`cat ${configPath}/gamelist.txt`);
  }

  input.value = output.replace(/\|/g, '\n');
};

const saveGamelist = async () => {
  const input = document.getElementById('gamelist_textarea');
  const formattedList = input.value.trim().replace(/\n+/g, '|');

  if (fileInterface) {
    fileInterface.write(`${configPath}/gamelist.txt`, formattedList);
  } else {
    await runCommand(`echo "${formattedList}" >${configPath}/gamelist.txt`);
  }

  const gamelist_save_success = getTranslation("toast.gamelist_save_success");
  toast(gamelist_save_success);
};

/* ======================== EVENT LISTENERS ======================== */
document.getElementById('save_log_btn').addEventListener('click', saveLog);
document.getElementById('edit_gamelist_btn').addEventListener('click', () => {
  disableBodyScroll();
  fetchGamelist();
});
document.getElementById('save_gamelist_btn').addEventListener('click', saveGamelist);
document.getElementById('create_shortcut_btn').addEventListener('click', createShortcut);
document.getElementById('donate_btn').addEventListener('click', () => openWebsite(donateUrl));
document.getElementById('encore_logo').addEventListener('click', () => openWebsite(officialWebsite));

document.querySelectorAll('.info-btn').forEach(btn => {
  btn.addEventListener('click', function() {
    openInfoModal(this);
  });
});

document.querySelectorAll('dialog').forEach(dialog => {
  dialog.addEventListener('close', enableBodyScroll);
});

/* ======================== INITIALIZATION ======================== */
getModuleVersion();
getCurrentProfile();
getKernelVersion();
getChipset();
getAndroidSDK();
getServiceState();
fetchCPUGovernors();
setupShortcut();
