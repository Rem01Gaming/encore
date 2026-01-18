<template>
  <div class="page game-settings-page h-full flex flex-col overflow-hidden bg-surface">
    <div class="max-w-3xl mx-auto h-full flex flex-col w-full">
      <!-- Header -->
      <div class="flex-none p-5 mb-2">
        <div class="flex items-center gap-4 mb-2">
          <button
            @click="$router.back()"
            class="text-on-surface hover:text-primary transition-colors"
          >
            <ArrowLeftIcon class="w-6 h-6 cursor-pointer rtl:rotate-180" />
          </button>
          <h1 class="text-xl font-semibold text-on-surface">
            {{ $t('game_settings.title') }}
          </h1>
        </div>
      </div>

      <!-- Settings Content -->
      <div class="scrollbar-hidden pb-25 md:pb-4 flex-1 min-h-0 overflow-y-scroll px-5">
        <div class="space-y-6">
          <h2 class="text-on-surface-variant text-sm font-medium tracking-wide">
            {{ $t('game_settings.application') }}
          </h2>

          <!-- App Info Section -->
          <div class="flex items-center gap-4">
            <img
              :src="currentApp.icon"
              @error="handleImageError"
              class="w-12 h-12 rounded-full object-cover"
              :alt="currentApp.appName"
            />
            <div class="flex-1 min-w-0">
              <h3 class="text-base font-medium text-on-surface truncate">
                {{ currentApp.appName || currentApp.packageName }}
              </h3>
              <p
                v-if="currentApp.appName && currentApp.appName !== currentApp.packageName"
                class="allow-copy text-sm text-on-surface-variant truncate mt-1"
              >
                {{ currentApp.packageName }}
              </p>
            </div>
          </div>

          <!-- Enable Tweaks Section -->
          <div class="space-y-4">
            <div class="flex items-center justify-between">
              <div class="flex items-center gap-1.5">
                <Candy class="text-primary shrink-0" />
                <div class="pl-3 pr-4">
                  <h3 class="text-base font-medium text-on-surface">
                    {{ $t('game_settings.enable_tweaks') }}
                  </h3>
                </div>
              </div>
              <ToggleSwitch
                class="opacity-100!"
                :model-value="appSettings.isEnabled"
                @update:model-value="toggleAppEnabled"
              />
            </div>
          </div>

          <!-- Divider -->
          <hr class="border-outline-variant opacity-40" />

          <!-- Preferences Section -->
          <div class="space-y-6">
            <h2
              class="text-on-surface-variant text-sm font-medium tracking-wide"
              :class="{ 'opacity-50': !appSettings.isEnabled }"
            >
              {{ $t('game_settings.preferences') }}
            </h2>

            <div class="space-y-6">
              <!-- Lite Mode -->
              <div
                class="flex items-center justify-between"
                :class="{ 'opacity-50': !appSettings.isEnabled || isGlobalLiteModeEnabled }"
              >
                <div class="flex items-center gap-1.5">
                  <Feather class="shrink-0 text-primary" />
                  <div class="pl-3 pr-4">
                    <h3 class="text-base font-medium text-on-surface">
                      {{ $t('game_settings.lite_mode') }}
                    </h3>
                    <p class="text-sm mt-1 text-on-surface-variant">
                      {{ $t('game_settings.lite_mode_description') }}
                    </p>
                  </div>
                </div>
                <ToggleSwitch
                  class="opacity-100!"
                  :model-value="liteModeSwitchValue"
                  :disabled="!appSettings.isEnabled || isGlobalLiteModeEnabled"
                  @update:model-value="toggleLiteMode"
                />
              </div>

              <!-- DND Mode -->
              <div
                class="flex items-center justify-between"
                :class="{ 'opacity-50': !appSettings.isEnabled }"
              >
                <div class="flex items-center gap-1.5">
                  <NoEntry class="text-primary shrink-0" />
                  <div class="pl-3 pr-4">
                    <h3 class="text-base font-medium text-on-surface">
                      {{ $t('game_settings.dnd_mode') }}
                    </h3>
                    <p class="text-sm text-on-surface-variant mt-1">
                      {{ $t('game_settings.dnd_mode_description') }}
                    </p>
                  </div>
                </div>
                <ToggleSwitch
                  class="opacity-100!"
                  :model-value="appSettings.enable_dnd"
                  :disabled="!appSettings.isEnabled"
                  @update:model-value="toggleDndMode"
                />
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, watch, onMounted, shallowRef } from 'vue'
import { useRoute, useRouter, onBeforeRouteLeave } from 'vue-router'
import { useGamesStore } from '@/stores/Games'
import { useEncoreConfigStore } from '@/stores/EncoreConfig'
import * as KernelSU from '@/helpers/KernelSU'

import ToggleSwitch from '@/components/ui/ToggleSwitch.vue'
import ArrowLeftIcon from '@/components/icons/ArrowLeft.vue'
import Candy from '@/components/icons/Candy.vue'
import Feather from '@/components/icons/Feather.vue'
import NoEntry from '@/components/icons/NoEntry.vue'

const route = useRoute()
const router = useRouter()
const gamesStore = useGamesStore()
const encoreConfigStore = useEncoreConfigStore()

const appSettings = shallowRef({ isEnabled: false, lite_mode: false, enable_dnd: false })

const currentApp = ref({})
const originalSettings = ref({})
const isLeaving = ref(false)
const saveTimeout = ref(null)
const isGlobalLiteModeEnabled = ref(false)

const liteModeSwitchValue = computed(() => {
  if (!appSettings.value.isEnabled) {
    return false
  }

  if (isGlobalLiteModeEnabled.value) {
    return true
  }

  return appSettings.value.lite_mode
})

watch(
  () => route.params.packageName,
  (newPackageName, oldPackageName) => {
    if (newPackageName && newPackageName !== oldPackageName) {
      loadAppData(newPackageName)
    }
  },
  { immediate: true },
)

onMounted(async () => {
  await loadGlobalConfig()
  loadAppData()
})

onBeforeRouteLeave(async (to, from, next) => {
  isLeaving.value = true
  clearTimeout(saveTimeout.value)
  await saveSettings()
  next()
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

function toggleAppEnabled() {
  const newValue = !appSettings.value.isEnabled
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

async function saveSettings() {
  if (JSON.stringify(appSettings.value) === JSON.stringify(originalSettings.value)) return

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
