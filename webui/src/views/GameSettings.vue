<template>
  <div class="page game-settings-page h-full flex flex-col overflow-hidden bg-surface">
    <div class="max-w-3xl mx-auto h-full flex flex-col w-full">
      <div class="flex-none p-5 mb-2 relative z-50">
        <div class="flex items-center justify-between">
          <div class="flex items-center gap-4">
            <button @click="$router.back()" class="text-on-surface hover:text-primary transition-colors">
              <ArrowLeftIcon class="w-6 h-6 cursor-pointer rtl:rotate-180" />
            </button>
            <h1 class="text-xl font-semibold text-on-surface">
              {{ $t('game_settings.title') }}
            </h1>
          </div>

          <DropdownMenu>
            <template #trigger>
              <button class="p-2 -mr-2 rounded-full text-on-surface hover:bg-on-surface/10 transition-colors">
                <DotsVertical />
              </button>
            </template>

            <template #content="{ close }">
              <MenuItem @click="() => { handleLaunchApp(); close(); }">
                <template #icon>
                  <OpenInNew :size="20" />
                </template>
                {{ $t('game_settings.launch_app') }}
              </MenuItem>

              <MenuItem @click="() => { handleOpenAppInfo(); close(); }">
                <template #icon>
                  <InformationOutline :size="20" />
                </template>
                {{ $t('game_settings.app_info') }}
              </MenuItem>
            </template>
          </DropdownMenu>

        </div>
      </div>

      <!-- Settings Content -->
      <div class="scrollbar-hidden pb-safe-nav flex-1 min-h-0 overflow-y-scroll px-5">
        <div class="space-y-6">
          <h2 class="text-on-surface-variant text-sm font-medium">
            {{ $t('game_settings.application') }}
          </h2>

          <!-- App Info Section -->
          <div class="flex items-center gap-4">
            <img :src="currentApp.icon" @error="handleImageError" class="w-10.5 h-10.5 rounded-full object-cover"
              :alt="currentApp.appName" />
            <div class="flex-1 min-w-0">
              <h3 class="text-base font-medium text-on-surface truncate">
                {{ currentApp.appName || currentApp.packageName }}
              </h3>
              <p v-if="currentApp.appName && currentApp.appName !== currentApp.packageName"
                class="allow-copy text-sm text-on-surface-variant truncate">
                {{ currentApp.packageName }}
              </p>
            </div>
          </div>

          <!-- Enable Tweaks Section -->
          <div class="flex items-center justify-between">
            <div class="flex items-center gap-8">
              <Candy :size="25" class="text-primary shrink-0" />
              <div class="pr-4">
                <h3 class="text-base font-medium text-on-surface">
                  {{ $t('game_settings.enable_tweaks') }}
                </h3>
              </div>
            </div>
            <ToggleSwitch class="opacity-100!" :model-value="appSettings.isEnabled"
              @update:model-value="toggleAppEnabled" />
          </div>

          <!-- Divider -->
          <hr class="border-outline-variant opacity-65 -mx-5" />

          <!-- Preferences Section -->
          <div class="space-y-6">
            <h2 class="text-on-surface-variant text-sm font-medium" :class="{ 'opacity-50': !appSettings.isEnabled }">
              {{ $t('game_settings.preferences') }}
            </h2>

            <div class="space-y-6">
              <!-- Lite Mode -->
              <div class="flex items-center justify-between"
                :class="{ 'opacity-50': !appSettings.isEnabled || isGlobalLiteModeEnabled }">
                <div class="flex items-center gap-8">
                  <Feather :size="25" class="shrink-0 text-primary" />
                  <div class="pr-4">
                    <h3 class="text-base font-medium text-on-surface">
                      {{ $t('game_settings.lite_mode') }}
                    </h3>
                    <p class="text-sm text-on-surface-variant">
                      {{ $t('game_settings.lite_mode_description') }}
                    </p>
                  </div>
                </div>
                <ToggleSwitch class="opacity-100!" :model-value="liteModeSwitchValue"
                  :disabled="!appSettings.isEnabled || isGlobalLiteModeEnabled" @update:model-value="toggleLiteMode" />
              </div>

              <!-- DND Mode -->
              <div class="flex items-center justify-between" :class="{ 'opacity-50': !appSettings.isEnabled }">
                <div class="flex items-center gap-8">
                  <NoEntry :size="25" class="text-primary shrink-0" />
                  <div class="pr-4">
                    <h3 class="text-base font-medium text-on-surface">
                      {{ $t('game_settings.dnd_mode') }}
                    </h3>
                    <p class="text-sm text-on-surface-variant">
                      {{ $t('game_settings.dnd_mode_description') }}
                    </p>
                  </div>
                </div>
                <ToggleSwitch class="opacity-100!" :model-value="appSettings.enable_dnd"
                  :disabled="!appSettings.isEnabled" @update:model-value="toggleDndMode" />
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, watch, onMounted, onBeforeUnmount, shallowRef } from 'vue'
import { useRoute, useRouter, onBeforeRouteLeave } from 'vue-router'
import { useGamesStore } from '@/stores/Games'
import { useEncoreConfigStore } from '@/stores/EncoreConfig'
import * as KernelSU from '@/helpers/KernelSU'
import { createDebouncedSave } from '@/helpers/Debounce'

import DropdownMenu from '@/components/ui/DropdownMenu.vue'
import MenuItem from '@/components/ui/MenuItem.vue'

import ToggleSwitch from '@/components/ui/ToggleSwitch.vue'
import ArrowLeftIcon from '@/components/icons/ArrowLeft.vue'
import Candy from '@/components/icons/Candy.vue'
import DotsVertical from '@/components/icons/DotsVertical.vue'
import Feather from '@/components/icons/Feather.vue'
import NoEntry from '@/components/icons/NoEntry.vue'
import InformationOutline from '@/components/icons/InformationOutline.vue'
import OpenInNew from '@/components/icons/OpenInNew.vue'

const route = useRoute()
const router = useRouter()
const gamesStore = useGamesStore()
const encoreConfigStore = useEncoreConfigStore()

const appSettings = shallowRef({ isEnabled: false, lite_mode: false, enable_dnd: false })

const currentApp = ref({})
const originalSettings = ref({})
const isGlobalLiteModeEnabled = ref(false)

const debouncedSave = createDebouncedSave(saveSettings, 500)

const liteModeSwitchValue = computed(() => {
  if (!appSettings.value.isEnabled) {
    return false
  }

  if (isGlobalLiteModeEnabled.value) {
    return true
  }

  return appSettings.value.lite_mode
})

watch(appSettings, () => {
  debouncedSave.trigger()
})

watch(
  () => route.params.packageName,
  async (newPackageName, oldPackageName) => {
    if (newPackageName && newPackageName !== oldPackageName) {
      await debouncedSave.flush()
      loadAppData(newPackageName)
    }
  },
  { immediate: true },
)

onMounted(async () => {
  await loadGlobalConfig()
})

onBeforeRouteLeave(async (to, from, next) => {
  await debouncedSave.flush()
  next()
})

onBeforeUnmount(async () => {
  await debouncedSave.flush()
})

async function loadGlobalConfig() {
  try {
    if (!encoreConfigStore.isLoaded) {
      await encoreConfigStore.loadConfig()
    }
    isGlobalLiteModeEnabled.value = encoreConfigStore.isLiteModeEnabled
  } catch (error) {
    console.error('Failed to load global config:', error)
    isGlobalLiteModeEnabled.value = false
  }
}

async function loadAppData(packageName = null) {
  const targetPackageName = packageName || route.params.packageName
  if (!targetPackageName) return router.push('/games')

  currentApp.value = {}
  appSettings.value = { isEnabled: false, lite_mode: false, enable_dnd: false }

  // First try to get from store
  const fromStore = gamesStore.userApps.find((a) => a.packageName === targetPackageName)
  if (fromStore) {
    currentApp.value = fromStore
  } else {
    try {
      // Try to get app info and icon
      const [info, icon] = await Promise.allSettled([
        KernelSU.getAppLabel(targetPackageName),
        KernelSU.getAppIcon(targetPackageName, 100),
      ])

      // Use results if successful, otherwise use fallbacks
      const appName = info.status === 'fulfilled' ? info.value : targetPackageName
      const appIcon =
        icon.status === 'fulfilled' && icon.value ? icon.value : '/fallback_app_icon.avif'

      currentApp.value = {
        packageName: targetPackageName,
        appName,
        icon: appIcon,
      }
    } catch {
      // Just use package name and fallback icon
      currentApp.value = {
        packageName: targetPackageName,
        appName: targetPackageName,
        icon: '/fallback_app_icon.avif',
      }
    }
  }

  loadAppSettings()
  originalSettings.value = { ...appSettings.value }
}

function loadAppSettings() {
  const cfg = gamesStore.gamelistConfig[currentApp.value.packageName] || {}
  appSettings.value = {
    isEnabled: currentApp.value.packageName in gamesStore.gamelistConfig,
    lite_mode: !!cfg.lite_mode,
    enable_dnd: !!cfg.enable_dnd,
  }
}

function toggleAppEnabled(newValue) {
  appSettings.value = {
    isEnabled: newValue,
    lite_mode: newValue ? appSettings.value.lite_mode : false,
    enable_dnd: newValue ? appSettings.value.enable_dnd : false,
  }
}

function toggleLiteMode() {
  if (isGlobalLiteModeEnabled.value) {
    return
  }

  if (appSettings.value.isEnabled) {
    appSettings.value = {
      ...appSettings.value,
      lite_mode: !appSettings.value.lite_mode,
    }
  }
}

function toggleDndMode() {
  if (appSettings.value.isEnabled) {
    appSettings.value = {
      ...appSettings.value,
      enable_dnd: !appSettings.value.enable_dnd,
    }
  }
}

function handleLaunchApp() {
  if (currentApp.value && currentApp.value.packageName) {
    KernelSU.launchApp(currentApp.value.packageName)
  }
}

function handleOpenAppInfo() {
  if (currentApp.value && currentApp.value.packageName) {
    KernelSU.openAppInfo(currentApp.value.packageName)
  }
}

async function saveSettings() {
  const settings = appSettings.value
  const original = originalSettings.value
  const settingsUnchanged =
    Object.keys(settings).length === Object.keys(original).length &&
    Object.keys(settings).every((key) => settings[key] === original[key])
  if (settingsUnchanged) return

  const pkg = currentApp.value.packageName
  if (!pkg) return

  try {
    if (appSettings.value.isEnabled) {
      await gamesStore.updateAppConfig(pkg, {
        lite_mode: appSettings.value.lite_mode,
        enable_dnd: appSettings.value.enable_dnd,
      })
    } else {
      await gamesStore.updateAppConfig(pkg, null)
    }

    originalSettings.value = { ...appSettings.value }
    console.log('Settings saved successfully for:', pkg)
  } catch (e) {
    console.error('saveSettings failed', e)
  }
}

function handleImageError(e) {
  e.target.src = '/app_icon_fallback.avif'
}
</script>
