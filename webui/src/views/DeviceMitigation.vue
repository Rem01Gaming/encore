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
            {{ $t('device_mitigation.title') }}
          </h1>

          <div class="aspect-3/2 rounded-3xl overflow-hidden -mx-1.5">
            <img src="/illustration/device_mitigation_poster.avif" class="w-full h-full object-cover" />
          </div>

          <div class="bg-primary-container rounded-3xl p-5 -mx-1.5">
            <div class="flex items-center justify-between">
              <div class="flex items-center gap-3">
                <h2 class="text-base font-medium text-on-primary-container">
                  {{ $t('device_mitigation.toggle_title') }}
                </h2>
              </div>
              <ToggleSwitch v-model="isDeviceMitigationEnabled" @update:modelValue="toggleDeviceMitigation" />
            </div>
          </div>

          <InformationOutlineIcon class="text-on-surface-variant my-6" :size="22" />
          <p class="text-sm text-on-surface-variant leading-relaxed">
            {{ $t('device_mitigation.brief') }}
          </p>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRouter, onBeforeRouteLeave } from 'vue-router'
import { useEncoreConfigStore } from '@/stores/EncoreConfig'

import ArrowLeftIcon from '@/components/icons/ArrowLeft.vue'
import ToggleSwitch from '@/components/ui/ToggleSwitch.vue'
import InformationOutlineIcon from '@/components/icons/InformationOutline.vue'

const router = useRouter()
const encoreConfigStore = useEncoreConfigStore()

const isDeviceMitigationEnabled = ref(false)
const hasUnsavedChanges = ref(false)

onMounted(async () => {
  try {
    if (!encoreConfigStore.isLoaded) {
      await encoreConfigStore.loadConfig()
    }
    isDeviceMitigationEnabled.value = encoreConfigStore.isDeviceMitigationEnabled
  } catch (error) {
    console.error('Failed to load device mitigation setting:', error)
  }
})

onBeforeRouteLeave(async (to, from, next) => {
  try {
    if (hasUnsavedChanges.value) {
      await encoreConfigStore.saveConfig()
      console.log('Settings saved via navigation guard')
      hasUnsavedChanges.value = false
    }
    next()
  } catch (error) {
    console.error('Failed to save on route leave:', error)
    next(false)
  }
})

async function toggleDeviceMitigation(enabled) {
  isDeviceMitigationEnabled.value = enabled

  try {
    if (!encoreConfigStore.isLoaded) {
      await encoreConfigStore.loadConfig()
    }

    encoreConfigStore.setDeviceMitigation(enabled)
    hasUnsavedChanges.value = true
    console.log(`Device mitigation ${enabled ? 'enabled' : 'disabled'}`)
  } catch (error) {
    console.error('Failed to set device mitigation:', error)
    isDeviceMitigationEnabled.value = encoreConfigStore.isDeviceMitigationEnabled
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
