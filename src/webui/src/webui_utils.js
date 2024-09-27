import { exec, toast } from 'kernelsu';
import encoreHappy from './assets/encore1.webp';
import encoreSleeping from './assets/encore2.webp';

async function getModuleVersion() {
  const { errno, stdout } = await exec('encore-utils get_module_version');
  if (errno === 0) {
    document.getElementById('moduleVer').textContent = stdout.trim();
  }
}

async function getServiceState() {
  const { errno, stdout } = await exec('encore-utils get_service_state');
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
  const { errno, stdout } = await exec('encore-utils get_service_pid');
  if (errno === 0) {
    document.getElementById('servicePID').textContent = "Service PID: " + stdout.trim();
  }
}

async function getSkipPrioritySwitch() {
  const { errno, stdout } = await exec('encore-utils get_skip_priority');
  if (errno === 0) {
    const switchElement = document.getElementById('skipPrioritySwitch');
    switchElement.checked = stdout.trim() === '1';
  }
}

async function getKillLogdSwitch() {
  const { errno, stdout } = await exec('encore-utils get_kill_logd');
  if (errno === 0) {
    const switchElement = document.getElementById('killLogdSwitch');
    switchElement.checked = stdout.trim() === '1';
  }
}

async function toggleKillLogdSwitch(isChecked) {
  const command = isChecked
    ? 'encore-utils kill_logd'
    : 'encore-utils disable_kill_logd';

  await exec(command);
}

async function restartService() {
  await exec('encore-utils restart_service');
  await getServiceState();
  await getServicePID();
}

async function changeCPUGovernor(governor) {
  const command = 'encore-utils set_default_cpugov ' + governor;
  await exec(command);
}

async function changePerfCPUGovernor(governor) {
  const command = 'encore-utils set_perf_cpugov ' + governor;
  await exec(command);
}

async function populateCPUGovernors() {
  const { errno: govErrno, stdout: govStdout } = await exec('encore-utils get_available_cpugov');
  if (govErrno === 0) {
    const governors = govStdout.trim().split(/\s+/);
    const selectElement1 = document.getElementById('cpuGovernor');
    const selectElement2 = document.getElementById('cpuGovernorPerf');

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

    const { errno: defaultErrno, stdout: defaultStdout } = await exec('encore-utils get_default_cpugov');
    if (defaultErrno === 0) {
      const defaultGovernor = defaultStdout.trim();
      selectElement1.value = defaultGovernor;
    }

    const { errno: perfErrno, stdout: perfStdout } = await exec('encore-utils get_perf_cpugov');
    if (perfErrno === 0) {
      const defaultPerfGovernor = perfStdout.trim();
      selectElement2.value = defaultPerfGovernor;
    }
  }
}

async function saveLogs() {
  await exec('encore-utils save_logs');
  toast('Logs have been saved on /sdcard/encore-logs');
}

async function openGamelistModal() {
  const modal = document.getElementById('gamelistModal');
  const input = document.getElementById('gamelistInput');

  const { errno, stdout } = await exec('encore-utils get_gamelist');
  if (errno === 0) {
    input.value = stdout.trim().replace(/\|/g, '\n');
  }

  modal.classList.remove('hidden');
}

async function saveGamelist() {
  const input = document.getElementById('gamelistInput');
  const gamelist = input.value.trim().replace(/\n+/g, '/');
  await exec(`encore-utils save_gamelist "${gamelist}"`);
  toast('Gamelist saved successfully.');
}

document.addEventListener('DOMContentLoaded', async (event) => {
  await getModuleVersion();
  await getServiceState();
  await getServicePID();
  await getKillLogdSwitch();
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

  document.getElementById('cpuGovernorPerf').addEventListener('change', async function() {
    await changePerfCPUGovernor(this.value);
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
});
