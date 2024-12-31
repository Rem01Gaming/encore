import { exec, toast } from 'kernelsu';
import encoreHappy from './assets/encore1.webp';
import encoreSleeping from './assets/encore2.webp';

let config_path = '/data/encore'

async function getModuleVersion() {
  const { errno, stdout } = await exec(
    `grep "version=" /data/adb/modules/encore/module.prop | awk -F'=' '{print $2}'`
  );
  if (errno === 0) {
    document.getElementById('moduleVer').textContent = stdout.trim();
  }
}

async function getServiceState() {
  const serviceStatusElement = document.getElementById('serviceStatus');
  const image = document.getElementById('imgEncore');
  const { errno, stdout } = await exec('pidof encored');
  if (errno === 0) {
    serviceStatusElement.textContent = "Working ✨";
    document.getElementById('servicePID').textContent = "Service PID: " + stdout.trim();
    image.src = encoreHappy;
  } else {
    serviceStatusElement.textContent = "Stopped 💤";
    document.getElementById('servicePID').textContent = "Service PID: null";
    image.src = encoreSleeping;
  }
}

async function getKillLogdSwitch() {
  const { errno, stdout } = await exec('cat kill_logd', { cwd: config_path });
  if (errno === 0) {
    const switchElement = document.getElementById('killLogdSwitch');
    switchElement.checked = stdout.trim() === '1';
  }
}

async function toggleKillLogdSwitch(isChecked) {
  const command = isChecked
    ? 'echo 1 >kill_logd'
    : 'echo 0 >kill_logd';
  toast('Reboot your device to take effect');
  await exec(command, { cwd: config_path });
}

async function restartService() {
  await exec('pkill encored && su -c /system/bin/encored');
  await getServiceState();
}

async function changeCPUGovernor(governor, config) {
  const command = `echo ${governor} >${config}`;
  await exec(command, { cwd: config_path });
}

async function fetchCPUGovernors() {
  const { errno: govErrno, stdout: govStdout } = await exec('chmod 644 scaling_available_governors && cat scaling_available_governors', { cwd: '/sys/devices/system/cpu/cpu0/cpufreq' });
  if (govErrno === 0) {
    const governors = govStdout.trim().split(/\s+/);
    const selectElement1 = document.getElementById('cpuGovernor');
    const selectElement2 = document.getElementById('cpuGovernorPowersave');

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

    const { errno: defaultErrno, stdout: defaultStdout } = await exec('[ -f custom_default_cpu_gov ] && cat custom_default_cpu_gov || cat default_cpu_gov', { cwd: config_path });
    if (defaultErrno === 0) {
      const defaultGovernor = defaultStdout.trim();
      selectElement1.value = defaultGovernor;
    }

    const { errno: powersaveErrno, stdout: powersaveStdout } = await exec('cat powersave_cpu_gov', { cwd: config_path });
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

async function openGamelistModal() {
  const modal = document.getElementById('gamelistModal');
  const input = document.getElementById('gamelistInput');

  const { errno, stdout } = await exec('cat gamelist.txt', { cwd: config_path });
  if (errno === 0) {
    input.value = stdout.trim().replace(/\|/g, '\n');
  }

  modal.classList.remove('hidden');
}

async function saveGamelist() {
  const input = document.getElementById('gamelistInput');
  const gamelist = input.value.trim().replace(/\n+/g, '/');
  await exec(`echo "${gamelist}" | tr '/' '|' >gamelist.txt`, { cwd: config_path });
  toast('Gamelist saved successfully.');
}

async function openWebsite() {
  await exec('/system/bin/am start -a android.intent.action.VIEW -d https://encore.rem01gaming.dev/');
}

document.addEventListener('DOMContentLoaded', async (event) => {
  await getModuleVersion();
  await getServiceState();
  await getKillLogdSwitch();
  await fetchCPUGovernors();

  document.getElementById('saveLogButton').addEventListener('click', async function() {
    await saveLog();
  });

  document.getElementById('restartServiceButton').addEventListener('click', async function() {
    await restartService();
  });

  document.getElementById('killLogdSwitch').addEventListener('change', async function() {
    await toggleKillLogdSwitch(this.checked);
  });

  document.getElementById('cpuGovernorPowersave').addEventListener('change', async function() {
    await changeCPUGovernor(this.value, "powersave_cpu_gov");
  });
  
  document.getElementById('cpuGovernor').addEventListener('change', async function() {
    await changeCPUGovernor(this.value, "custom_default_cpu_gov");
  });

  document.getElementById('editGamelistButton').addEventListener('click', function() {
    openGamelistModal();
  });

  document.getElementById('cancelButton').addEventListener('click', function() {
    document.getElementById('gamelistModal').classList.add('hidden');
  });

  document.getElementById('saveGamelistButton').addEventListener('click', async function() {
    await saveGamelist();
    document.getElementById('gamelistModal').classList.add('hidden');
  });

  document.getElementById('imgEncore').addEventListener('click', async function() {
    await openWebsite();
  });
});
