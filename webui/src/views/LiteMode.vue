<template>
  <div class="page h-full flex flex-col overflow-hidden bg-surface">
    <div class="max-w-3xl mx-auto h-full flex flex-col w-full">
      <div class="flex-none p-5 pb-3">
        <div class="flex items-center gap-4 mb-2">
          <button @click="goBack" class="text-on-surface transition-colors">
            <ArrowLeftIcon class="w-6 h-6 cursor-pointer rtl:rotate-180" />
          </button>
        </div>
      </div>

      <div class="scrollbar-hidden pb-safe-nav flex-1 min-h-0 overflow-y-scroll px-5">
        <div class="space-y-6">
          <h1 class="text-4xl text-on-surface mt-12 mb-6">
            {{ $t('lite_mode.title') }}
          </h1>

          <div class="aspect-3/2 rounded-3xl overflow-hidden -mx-1.5">
            <video ref="videoElement" preload="auto" poster="/illustration/lite_mode_poster.avif"
              class="w-full h-full object-cover" autoplay loop muted playsinline webkit-playsinline>
              <source src="/illustration/lite_mode.webm" type="video/webm" />
            </video>
          </div>

          <div class="bg-primary-container rounded-3xl p-5 -mx-1.5">
            <div class="flex items-center justify-between">
              <div class="flex items-center gap-3">
                <h2 class="text-base font-medium text-on-primary-container">
                  {{ $t('lite_mode.toggle_title') }}
                </h2>
              </div>
              <ToggleSwitch v-model="isLiteModeEnabled" @update:modelValue="toggleLiteMode" />
            </div>
          </div>

          <InformationOutlineIcon class="text-on-surface-variant my-6" :size="22" />
          <p class="text-sm text-on-surface-variant leading-relaxed">
            {{ $t('lite_mode.brief') }}
          </p>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, onActivated, onDeactivated, onBeforeUnmount } from 'vue'
import { useRouter, onBeforeRouteLeave } from 'vue-router'
import { useEncoreConfigStore } from '@/stores/EncoreConfig'

import ArrowLeftIcon from '@/components/icons/ArrowLeft.vue'
import ToggleSwitch from '@/components/ui/ToggleSwitch.vue'
import InformationOutlineIcon from '@/components/icons/InformationOutline.vue'

const router = useRouter()
const encoreConfigStore = useEncoreConfigStore()

const isLiteModeEnabled = ref(false)
const videoElement = ref(null)
const hasUnsavedChanges = ref(false)
const wakeLock = ref(null)

const playVideo = async () => {
  if (!videoElement.value) return

  try {
    videoElement.value.muted = true
    await videoElement.value.play()
  } catch (err) {
    console.warn('Initial playback failed, retrying after transition...', err)

    setTimeout(async () => {
      try {
        if (videoElement.value) {
          await videoElement.value.play()
        }
      } catch (retryErr) {
        console.error('Playback blocked by WebView policy:', retryErr)
      }
    }, 300)
  }
}

const requestWakeLock = async () => {
  if ('wakeLock' in navigator) {
    try {
      wakeLock.value = await navigator.wakeLock.request('screen')
    } catch (err) {
      console.error(`${err.name}, ${err.message}`)
    }
  }
}

const releaseWakeLock = async () => {
  if (wakeLock.value !== null) {
    await wakeLock.value.release()
    wakeLock.value = null
  }
}

const handleVisibilityChange = async () => {
  if (document.visibilityState === 'visible') {
    await requestWakeLock()
    playVideo()
  }
}

onMounted(async () => {
  try {
    if (!encoreConfigStore.isLoaded) {
      await encoreConfigStore.loadConfig()
    }
    isLiteModeEnabled.value = encoreConfigStore.isLiteModeEnabled

    if (videoElement.value) {
      videoElement.value.addEventListener('canplay', playVideo, { once: true })
    }

    document.addEventListener('visibilitychange', handleVisibilityChange)
    await requestWakeLock()
  } catch (error) {
    console.error('Failed to load lite mode setting:', error)
  }
})

onActivated(async () => {
  await requestWakeLock()
  playVideo()
})

onDeactivated(async () => {
  await releaseWakeLock()
  if (videoElement.value) {
    videoElement.value.pause()
  }
})

onBeforeRouteLeave(async (to, from, next) => {
  try {
    if (hasUnsavedChanges.value) {
      await encoreConfigStore.saveConfig()
      hasUnsavedChanges.value = false
    }
    next()
  } catch (error) {
    console.error('Failed to save on route leave:', error)
    next(false)
  }
})

onBeforeUnmount(async () => {
  document.removeEventListener('visibilitychange', handleVisibilityChange)
  if (videoElement.value) {
    videoElement.value.removeEventListener('canplay', playVideo)
  }
  await releaseWakeLock()
})

async function toggleLiteMode(enabled) {
  isLiteModeEnabled.value = enabled
  try {
    if (!encoreConfigStore.isLoaded) {
      await encoreConfigStore.loadConfig()
    }
    encoreConfigStore.setLiteMode(enabled)
    hasUnsavedChanges.value = true
  } catch (error) {
    console.error('Failed to set lite mode:', error)
    isLiteModeEnabled.value = encoreConfigStore.isLiteModeEnabled
  }
}

function goBack() {
  encoreConfigStore
    .saveConfig()
    .then(() => {
      hasUnsavedChanges.value = false
      router.back()
    })
    .catch((error) => {
      console.error('Failed to save on goBack:', error)
      router.back()
    })
}
</script>
