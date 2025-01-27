import { exec, toast } from 'kernelsu';
import encoreHappy from './encore_happy.webp';
import encoreSleeping from './encore_sleeping.webp';

let config_path = '/data/encore'

async function getModuleVersion() {
  try {
    const { errno, stdout } = await exec(
      `grep "version=" /data/adb/modules/encore/module.prop | awk -F'=' '{print $2}'`
    );
    if (errno === 0) {
      document.getElementById('module_version').textContent = stdout.trim();
    }
  } catch (error) {
    if (typeof ksu !== 'undefined' && ksu.mmrl) {
      mmrl_denied.showModal();
    }
  }
}

async function getKernelVersion() {
  const { errno, stdout } = await exec(
    `uname -r -m`
  );
  if (errno === 0) {
    document.getElementById('kernel_version').textContent = stdout.trim();
  }
}

async function getChipset() {
  const { errno, stdout } = await exec(
    `getprop ro.board.platform`
  );
  if (errno === 0) {
    document.getElementById('chipset_name').textContent = stdout.trim();
  }
}

async function getAndroidSDK() {
  const { errno, stdout } = await exec(
    `getprop ro.build.version.sdk`
  );
  if (errno === 0) {
    document.getElementById('android_sdk').textContent = stdout.trim();
  }
}

async function getServiceState() {
  const status = document.getElementById('daemon_status');
  const image = document.getElementById('encore_pics');
  const pid = document.getElementById('daemon_pid');

  const { stdout } = await exec('busybox pidof encored || pidof encored || echo null');
  pid.textContent = "Daemon PID: " + stdout.trim();

  if (stdout.trim() === "null") {
    status.textContent = "Stopped 💤";
    image.src = encoreSleeping;
  } else {
    status.textContent = "Working ✨";
    image.src = encoreHappy;
  }
}

async function getKillLogdSwitch() {
  const { errno, stdout } = await exec(`cat ${config_path}/kill_logd`);
  if (errno === 0) {
    const switchElement = document.getElementById('kill_logd_switch');
    switchElement.checked = stdout.trim() === '1';
  }
}

async function toggleKillLogdSwitch(isChecked) {
  const command = isChecked
    ? `echo 1 >${config_path}/kill_logd`
    : `echo 0 >${config_path}/kill_logd`;
  toast('Reboot your device to take effect');
  await exec(command);
}

async function getBypassChargingSwitch() {
  const { errno, stdout } = await exec(`cat ${config_path}/bypass_charging`);
  if (errno === 0) {
    const switchElement = document.getElementById('bypass_charging_switch');
    switchElement.checked = stdout.trim() === '1';
  }
}

async function toggleBypassChargingSwitch(isChecked) {
  const command = isChecked
    ? `echo 1 >${config_path}/bypass_charging`
    : `echo 0 >${config_path}/bypass_charging`;
  await exec(command);
}

async function restartService() {
  await exec('pkill encored');
  await exec('su -c encored');
  await getServiceState();
}

async function changeCPUGovernor(governor, config) {
  await exec(`echo ${governor} >${config_path}/${config}`);
}

async function fetchCPUGovernors() {
  const { errno: govErrno, stdout: govStdout } = await exec('chmod 644 scaling_available_governors && cat scaling_available_governors', { cwd: '/sys/devices/system/cpu/cpu0/cpufreq' });
  if (govErrno === 0) {
    const governors = govStdout.trim().split(/\s+/);
    const selectElement1 = document.getElementById('default_cpu_gov');
    const selectElement2 = document.getElementById('powersave_cpu_gov');

    selectElement1.innerHTML = '';
    selectElement2.innerHTML = '';

    governors.forEach(gov => {
      const option1 = document.createElement('option');
      option1.value = gov;
      option1.textContent = gov;
      selectElement1.appendChild(option1);

      const option2 = document.createElement('option');
      option2.value = gov;
      option2.textContent = gov;
      selectElement2.appendChild(option2);
    });

    const { errno: defaultErrno, stdout: defaultStdout } = await exec(`[ -f ${config_path}/custom_default_cpu_gov ] && cat ${config_path}/custom_default_cpu_gov || cat ${config_path}/default_cpu_gov`);
    if (defaultErrno === 0) {
      const defaultGovernor = defaultStdout.trim();
      selectElement1.value = defaultGovernor;
    }

    const { errno: powersaveErrno, stdout: powersaveStdout } = await exec(`cat ${config_path}/powersave_cpu_gov`);
    if (powersaveErrno === 0) {
      const defaultPowersaveGovernor = powersaveStdout.trim();
      selectElement2.value = defaultPowersaveGovernor;
    }
  }
}

async function saveLog() {
  await exec('encore_utility save_logs');
  toast('Logs have been saved on /sdcard/encore_log');
}

async function fetchGamelist() {
  const input = document.getElementById('gamelist_textarea');
  const { errno, stdout } = await exec(`cat ${config_path}/gamelist.txt`);
  if (errno === 0) {
    input.value = stdout.trim().replace(/\|/g, '\n');
  }
}

async function saveGamelist() {
  const input = document.getElementById('gamelist_textarea');
  const gamelist = input.value.trim().replace(/\n+/g, '/');
  await exec(`echo "${gamelist}" | tr '/' '|' >${config_path}/gamelist.txt`);
  toast('Gamelist saved successfully.');
}

async function openWebsite(link) {
  await exec(`/system/bin/am start -a android.intent.action.VIEW -d ${link}`);
}

getModuleVersion();
getKernelVersion();
getChipset();
getAndroidSDK();
getServiceState();
getKillLogdSwitch();
getBypassChargingSwitch();
fetchCPUGovernors();

document.getElementById('save_log_btn').addEventListener('click', async function() {
  saveLog();
});

document.getElementById('restart_daemon_btn').addEventListener('click', async function() {
  restartService();
});

document.getElementById('kill_logd_switch').addEventListener('change', async function() {
  toggleKillLogdSwitch(this.checked);
});

document.getElementById('bypass_charging_switch').addEventListener('change', async function() {
  toggleBypassChargingSwitch(this.checked);
});

document.getElementById('powersave_cpu_gov').addEventListener('change', async function() {
  changeCPUGovernor(this.value, "powersave_cpu_gov");
});
  
document.getElementById('default_cpu_gov').addEventListener('change', async function() {
  changeCPUGovernor(this.value, "custom_default_cpu_gov");
});

document.getElementById('edit_gamelist_btn').addEventListener('click', function() {
  fetchGamelist();
});

document.getElementById('save_gamelist_btn').addEventListener('click', async function() {
  saveGamelist();
});

document.getElementById('encore_pics').addEventListener('click', async function() {
  openWebsite("https://encore.rem01gaming.dev/");
});

document.getElementById('donate_btn').addEventListener('click', async function() {
  openWebsite("https://t.me/rem01schannel/670");
});
