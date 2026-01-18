import { defineStore } from 'pinia'
import { ref } from 'vue'
import { exec } from 'kernelsu'
import { moduleInterface } from '@/helpers/WXInterfaces'
import * as KernelSU from '@/helpers/KernelSU'

const configPath = '/data/adb/.config/encore'
const modPath = '/data/adb/modules/encore'

export const useHomeStore = defineStore('home', () => {
  const daemonPidRaw = ref('')
  const moduleVersion = ref('')
  const currentProfileRaw = ref('')
  const kernelVersion = ref('')
  const chipsetName = ref('')
  const androidSDK = ref('')
  const daemonStatusRaw = ref('loading') // 'loading', 'running', 'stopped', 'error'
  const daemonError = ref('')
  const logoImage = ref('/encore_sleeping.avif')
  const isInitialized = ref(false)

  let profileInterval = null

  // Actions
  async function initializeData() {
    if (isInitialized.value) return

    await Promise.all([
      getServiceState(),
      getAndroidSDK(),
      getModuleVersion(),
      getCurrentProfile(),
      getKernelVersion(),
      getChipset(),
    ])

    startProfileMonitoring()
    isInitialized.value = true
  }

  function startProfileMonitoring() {
    stopProfileMonitoring()

    profileInterval = setInterval(() => {
      getCurrentProfile()
    }, 1000)
  }

  function stopProfileMonitoring() {
    if (profileInterval) {
      clearInterval(profileInterval)
      profileInterval = null
    }
  }

  async function getServiceState() {
    try {
      if (!KernelSU.isKSUWebUI()) {
        throw new Error('Not running on KSU WebUI')
      }

      const { errno, stdout } = await exec('/system/bin/toybox pidof encored')
      if (errno !== 0) {
        setDaemonStopped()
        return
      }

      daemonPidRaw.value = stdout.trim()
      daemonStatusRaw.value = 'running'
      daemonError.value = ''
      logoImage.value = '/encore_happy.avif'
    } catch (error) {
      setDaemonError(error.message)
    }
  }

  function setDaemonStopped() {
    daemonStatusRaw.value = 'stopped'
    daemonPidRaw.value = ''
    daemonError.value = ''
    logoImage.value = '/encore_sleeping.avif'
  }

  function setDaemonError(message) {
    daemonStatusRaw.value = 'error'
    daemonError.value = message
    logoImage.value = '/encore_sleeping.avif'
  }

  async function getAndroidSDK() {
    try {
      if (!KernelSU.isKSUWebUI()) {
        throw new Error('Not running on KSU WebUI')
      }

      let sdk
      if (window.$encore) {
        sdk = moduleInterface.getSdk()
      } else {
        const { stdout } = await exec('getprop ro.build.version.sdk')
        sdk = stdout.trim()
      }

      androidSDK.value = sdk
    } catch (error) {
      androidSDK.value = 'unknown'
    }
  }

  async function getModuleVersion() {
    try {
      const propPath = `${modPath}/module.prop`
      const content = await KernelSU.readFile(propPath)
      const match = content.match(/^version=(.*)$/m)
      moduleVersion.value = match ? match[1].trim() : 'unknown'
    } catch (error) {
      moduleVersion.value = 'unknown'
    }
  }

  async function getCurrentProfile() {
    try {
      const output = await KernelSU.readFile(`${configPath}/current_profile`)
      currentProfileRaw.value = getProfileKey(output.trim())
    } catch (error) {
      currentProfileRaw.value = 'unknown'
    }
  }

  function getProfileKey(profileCode) {
    const profileMap = {
      0: 'initializing',
      1: 'performance',
      2: 'balanced',
      3: 'powersave',
    }
    return profileMap[profileCode] || 'unknown'
  }

  async function getKernelVersion() {
    try {
      if (!KernelSU.isKSUWebUI()) {
        throw new Error('Not running on KSU WebUI')
      }

      const { stdout } = await exec('uname -r -m')
      kernelVersion.value = stdout.trim()
    } catch (error) {
      kernelVersion.value = 'unknown'
    }
  }

  async function getChipset() {
    try {
      if (!KernelSU.isKSUWebUI()) {
        throw new Error('Not running on KSU WebUI')
      }

      const { stdout } = await exec('getprop ro.board.platform')
      const chipset = stdout.trim()
      const brand = await getChipsetBrand()

      chipsetName.value = `${brand} ${chipset}`
    } catch (error) {
      chipsetName.value = 'unknown'
    }
  }

  async function getChipsetBrand() {
    try {
      const soc = await KernelSU.readFile(`${configPath}/soc_recognition`)
      const brands = {
        1: 'MediaTek',
        2: 'Snapdragon',
        3: 'Exynos',
        4: 'Unisoc',
        5: 'Tensor',
        6: 'Intel',
        7: 'Tegra',
        8: 'Kirin',
      }
      return brands[soc] || ''
    } catch (error) {
      return ''
    }
  }

  return {
    // Raw state
    daemonPidRaw,
    moduleVersion,
    currentProfileRaw,
    kernelVersion,
    chipsetName,
    androidSDK,
    daemonStatusRaw,
    daemonError,
    logoImage,
    isInitialized,

    // Actions
    initializeData,
    stopProfileMonitoring,
    getServiceState,
    getAndroidSDK,
    getModuleVersion,
    getCurrentProfile,
    getKernelVersion,
    getChipset,
  }
})
