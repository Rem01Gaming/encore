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
import encoreHappy from '../assets/encore_happy.webp';
import encoreSleeping from '../assets/encore_sleeping.webp';

const configPath = '/data/encore';
const binPath = '/data/adb/modules/encore/system/bin';

// Helper function for executing shell commands
const runCommand = async (cmd, cwd = null) => {
  const { errno, stdout, stderr } = await exec(cmd, cwd ? { cwd } : {});
  return errno === 0 ? stdout.trim() : { error: stderr };
};

// Display an error modal
const showErrorModal = (title, msg) => {
  document.getElementById('error_title').textContent = title;
  document.getElementById('error_msg').textContent = msg;
  error_modal.showModal();
};

/* ======================== SYSTEM INFO ======================== */
const getModuleVersion = async () => {
  const output = await runCommand(`[ -f module.prop ] && awk -F'=' '/version=/ {print $2}' module.prop || echo null`, '/data/adb/modules/encore');
  if (output === 'null') {
    showErrorModal("Unauthorized Modification", "This module may have been modified by a third party. For your security, please download the official version from https://encore.rem01gaming.dev/");
    toast("Unauthorized modification detected.");
  } else {
    document.getElementById('module_version').textContent = output;
  }
};

const getCurrentProfile = async () => {
  const output = await runCommand('cat /dev/encore_mode');
  let profile = "Unknown";

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
  const soc = await runCommand(`cat /data/encore/soc_recognition`);
  let brand = "Unknown";
  
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
  document.getElementById('android_sdk').textContent = await runCommand(`getprop ro.build.version.sdk`);
};

/* ======================== SERVICE MANAGEMENT ======================== */
const getServiceState = async () => {
  const status = document.getElementById('daemon_status');
  const image = document.getElementById('encore_logo');
  const pidElem = document.getElementById('daemon_pid');

  const pid = await runCommand('/system/bin/toybox pidof encored || echo null');
  pidElem.textContent = `Daemon PID: ${pid}`;

  if (pid === "null") {
    status.textContent = "Stopped 💤";
    image.src = encoreSleeping;
  } else {
    status.textContent = "Working ✨";
    image.src = encoreHappy;
  }
};

/* ======================== CONFIGURATION ======================== */
const toggleConfig = async (file, isChecked, rebootMessage = false) => {
  await runCommand(`echo ${isChecked ? '1' : '0'} >${configPath}/${file}`);
  if (rebootMessage) toast('Reboot your device to apply changes');
};

const setupSwitch = async (id, file, rebootMessage = false) => {
  const switchElement = document.getElementById(id);
  switchElement.checked = await runCommand(`cat ${configPath}/${file}`) === '1';
  switchElement.addEventListener('change', () => toggleConfig(file, switchElement.checked, rebootMessage));
};

// Initialize switches
setupSwitch('lite_mode_switch', 'lite_mode');
setupSwitch('dnd_switch', 'dnd_gameplay');
setupSwitch('device_mitigation_switch', 'device_mitigation');

/* ======================== CPU GOVERNOR MANAGEMENT ======================== */
const changeCPUGovernor = async (governor, config) => {
  await runCommand(`echo ${governor} >${configPath}/${config}`);
  if (config === "powersave_cpu_gov") {
    await runCommand(`[ "$(</dev/encore_mode)" -eq 3 ] && encore_utility change_cpu_gov ${governor}`);
  } else if (config === "custom_default_cpu_gov") {
    await runCommand(`[ "$(</dev/encore_mode)" -eq 2 ] && encore_utility change_cpu_gov ${governor}`);
  }
};

const fetchCPUGovernors = async () => {
  const governors = (await runCommand('cat scaling_available_governors', '/sys/devices/system/cpu/cpu0/cpufreq')).split(/\s+/);
  ['default_cpu_gov', 'powersave_cpu_gov'].forEach(id => {
    const select = document.getElementById(id);
    select.innerHTML = governors.map(gov => `<option value="${gov}">${gov}</option>`).join('');
    select.addEventListener('change', () => changeCPUGovernor(select.value, id.includes('powersave') ? "powersave_cpu_gov" : "custom_default_cpu_gov"));
  });

  document.getElementById('default_cpu_gov').value = await runCommand(`[ -f ${configPath}/custom_default_cpu_gov ] && cat ${configPath}/custom_default_cpu_gov || cat ${configPath}/default_cpu_gov`);
  document.getElementById('powersave_cpu_gov').value = await runCommand(`cat ${configPath}/powersave_cpu_gov`);
};

/* ======================== GAMELIST MANAGEMENT ======================== */
const fetchGamelist = async () => {
  const input = document.getElementById('gamelist_textarea');
  const output = await runCommand(`cat /dev/encore_gamelist`);
  if (output.error) {
    showErrorModal("Unable to fetch Gamelist", output.error);
  } else {
    input.value = output.replace(/\|/g, '\n');
  }
};

const saveGamelist = async () => {
  const input = document.getElementById('gamelist_textarea');
  const formattedList = input.value.trim().replace(/\n+/g, '|');
  const result = await runCommand(`echo "${formattedList}" | tee /data/encore/gamelist.txt /dev/encore_gamelist >/dev/null`);
  result.error ? showErrorModal("Unable to save Gamelist", result.error) : toast('Gamelist saved successfully.');
};

/* ======================== UTILITIES ======================== */
const saveLog = async () => {
  const output = await runCommand(`${binPath}/encore_utility save_logs`);
  output.error ? showErrorModal("Unable to save logs", output.error) : toast(`Logs saved at ${output}`);
};

const openWebsite = async (link) => {
  const result = await runCommand(`am start -a android.intent.action.VIEW -d ${link}`);
  if (result.error) showErrorModal("Unable to open browser", result.error);
};

/* ======================== EVENT LISTENERS ======================== */
document.getElementById('save_log_btn').addEventListener('click', saveLog);
document.getElementById('edit_gamelist_btn').addEventListener('click', fetchGamelist);
document.getElementById('save_gamelist_btn').addEventListener('click', saveGamelist);
document.getElementById('donate_btn').addEventListener('click', () => openWebsite("https://t.me/rem01schannel/670"));
document.getElementById('encore_logo').addEventListener('click', () => openWebsite("https://encore.rem01gaming.dev/"));

/* ======================== INITIALIZATION ======================== */
getModuleVersion();
getCurrentProfile();
getKernelVersion();
getChipset();
getAndroidSDK();
getServiceState();
fetchCPUGovernors();
