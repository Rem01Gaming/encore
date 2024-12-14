import { exec, toast } from 'kernelsu';
import encoreHappy from './assets/encore1.webp';
import encoreSleeping from './assets/encore2.webp';

async function getModuleVersion() {
  const { errno, stdout } = await exec(
    `grep "version=" /data/adb/modules/encore/module.prop | awk -F'=' '{print $2}'`
  );
  if (errno === 0) {
    document.getElementById('moduleVer').textContent = stdout.trim();
  }
}

async function getServiceState() {
  const { errno, stdout } = await exec('[ ! -z $(sudo pidof encored) ] && echo 1 || echo 0');
  if (errno === 0) {
    const serviceStatusElement = document.getElementById('serviceStatus');
    const image = document.getElementById('imgEncore');
    if (stdout.trim() === '1') {
       serviceStatusElement.textContent = "Working ✨";
       image.src = encoreHappy;
    } else {
       serviceStatusElement.textContent = "Stopped 💤";
       image.src = encoreSleeping;
    }
  }
}

async function getServicePID() {
  const { errno, stdout } = await exec('pidof -s encored || echo null');
  if (errno === 0) {
    document.getElementById('servicePID').textContent = "Service PID: " + stdout.trim();
  }
}

async function getKillLogdSwitch() {
  const { errno, stdout } = await exec('cat /data/encore/kill_logd');
  if (errno === 0) {
    const switchElement = document.getElementById('killLogdSwitch');
    switchElement.checked = stdout.trim() === '1';
  }
}

async function getGamePreloadSwitch() {
  const { errno, stdout } = await exec('cat /data/encore/game_preload');
  if (errno === 0) {
    const switchElement = document.getElementById('gamePreloadSwitch');
    switchElement.checked = stdout.trim() === '1';
  }
}

async function toggleKillLogdSwitch(isChecked) {
  const command = isChecked
    ? 'echo 1 >/data/encore/kill_logd'
    : 'echo 0 >/data/encore/kill_logd';
  toast('Reboot your device to take effect');
  await exec(command);
}

async function toggleGamePreloadSwitch(isChecked) {
  const command = isChecked
    ? 'echo 1 >/data/encore/game_preload'
    : 'echo 0 >/data/encore/game_preload';
  await exec(command);
}

async function restartService() {
  await exec('pkill encored && su -c /system/bin/encored');
  await getServiceState();
  await getServicePID();
}

async function changeCPUGovernor(governor) {
  const command = 'echo ' + governor + ' >/data/encore/custom_default_cpu_gov';
  await exec(command);
}

async function changePowersaveCPUGovernor(governor) {
  const command = 'echo ' + governor + ' >/data/encore/powersave_cpu_gov';
  await exec(command);
}

async function populateCPUGovernors() {
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

    const { errno: defaultErrno, stdout: defaultStdout } = await exec('[ -f custom_default_cpu_gov ] && cat custom_default_cpu_gov || cat default_cpu_gov', { cwd: '/data/encore' });
    if (defaultErrno === 0) {
      const defaultGovernor = defaultStdout.trim();
      selectElement1.value = defaultGovernor;
    }

    const { errno: powersaveErrno, stdout: powersaveStdout } = await exec('cat /data/encore/powersave_cpu_gov');
    if (powersaveErrno === 0) {
      const defaultPowersaveGovernor = powersaveStdout.trim();
      selectElement2.value = defaultPowersaveGovernor;
    }
  }
}

async function saveLogs() {
  await exec('encore_utility save_logs');
  toast('Logs have been saved on /sdcard/encore_log');
}

async function openGamelistModal() {
  const modal = document.getElementById('gamelistModal');
  const input = document.getElementById('gamelistInput');

  const { errno, stdout } = await exec('cat /data/encore/gamelist.txt');
  if (errno === 0) {
    input.value = stdout.trim().replace(/\|/g, '\n');
  }

  modal.classList.remove('hidden');
}

async function saveGamelist() {
  const input = document.getElementById('gamelistInput');
  const gamelist = input.value.trim().replace(/\n+/g, '/');
  await exec(`echo "${gamelist}" | tr '/' '|' >/data/encore/gamelist.txt`);
  toast('Gamelist saved successfully.');
}

async function openWebsite() {
  await exec('/system/bin/am start -a android.intent.action.VIEW -d https://encore.rem01gaming.dev/');
}

document.addEventListener('DOMContentLoaded', async (event) => {
  await getModuleVersion();
  await getServiceState();
  await getServicePID();
  await getKillLogdSwitch();
  await getGamePreloadSwitch();
  await populateCPUGovernors();

  document.getElementById('saveLogsButton').addEventListener('click', async function() {
    await saveLogs();
  });

  document.getElementById('restartServiceButton').addEventListener('click', async function() {
    await restartService();
  });

  document.getElementById('killLogdSwitch').addEventListener('change', async function() {
    await toggleKillLogdSwitch(this.checked);
  });

  document.getElementById('gamePreloadSwitch').addEventListener('change', async function() {
    await toggleGamePreloadSwitch(this.checked);
  });

  document.getElementById('cpuGovernorPowersave').addEventListener('change', async function() {
    await changePowersaveCPUGovernor(this.value);
  });
  
  document.getElementById('cpuGovernor').addEventListener('change', async function() {
    await changeCPUGovernor(this.value);
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
