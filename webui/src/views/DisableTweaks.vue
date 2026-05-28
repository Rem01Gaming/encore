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
            {{ $t('disable_tweaks.title') }}
          </h1>

          <!-- <div class="aspect-3/2 rounded-3xl overflow-hidden -mx-1.5">
            <img src="/illustration/device_mitigation_poster.avif" class="w-full h-full object-cover" />
          </div> -->

          <div class="bg-primary-container rounded-3xl p-5 -mx-1.5">
            <div class="flex items-center justify-between">
              <div class="flex items-center gap-3">
                <h2 class="text-base font-medium text-on-primary-container">
                  {{ $t('disable_tweaks.toggle_title') }}
                </h2>
              </div>
              <ToggleSwitch v-model="isDisableTweaksEnabled" @update:modelValue="toggleDisableTweaks" />
            </div>
          </div>

          <InformationOutlineIcon class="text-on-surface-variant my-6" :size="22" />
          <p class="text-sm text-on-surface-variant leading-relaxed">
            {{ $t('disable_tweaks.brief') }}
          </p>
        </div>
      </div>
    </div>

    <Modal :show="showRebootModal" :title="$t('reboot_modal.title')" @close="closeRebootModal"
      :closeOnOutsideClick="false">
      <div class="px-4 pb-2">
        <div class="flex flex-col items-center gap-4 py-6">
          <RefreshIcon :size="48" class="text-primary" />
          <p class="text-on-surface-variant text-sm text-center">{{ $t('reboot_modal.description') }}</p>
        </div>
      </div>

      <template #actions>
        <div class="flex gap-2">
          <button @click="skipReboot"
            class="px-4 py-2 text-sm font-medium text-primary hover:bg-primary/10 rounded-full transition-colors">
            {{ $t('reboot_modal.later') }}
          </button>
          <button @click="rebootDevice"
            class="px-4 py-2 text-sm font-medium text-primary hover:bg-primary/10 rounded-full transition-colors">
            {{ $t('reboot_modal.reboot') }}
          </button>
        </div>
      </template>
    </Modal>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRouter, onBeforeRouteLeave } from 'vue-router'
import { useEncoreConfigStore } from '@/stores/EncoreConfig'
import { exec } from 'kernelsu'

import ArrowLeftIcon from '@/components/icons/ArrowLeft.vue'
import RefreshIcon from '@/components/icons/Refresh.vue'
import ToggleSwitch from '@/components/ui/ToggleSwitch.vue'
import InformationOutlineIcon from '@/components/icons/InformationOutline.vue'
import Modal from '@/components/ui/Modal.vue'

const router = useRouter()
const encoreConfigStore = useEncoreConfigStore()

const isDisableTweaksEnabled = ref(false)
const initialValue = ref(false)
const hasUnsavedChanges = ref(false)
const showRebootModal = ref(false)

onMounted(async () => {
  try {
    if (!encoreConfigStore.isLoaded) {
      await encoreConfigStore.loadConfig()
    }
    isDisableTweaksEnabled.value = encoreConfigStore.isDisableTweaksEnabled
    initialValue.value = encoreConfigStore.isDisableTweaksEnabled
  } catch (error) {
    console.error('Failed to load disable tweaks setting:', error)
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

async function toggleDisableTweaks(enabled) {
  isDisableTweaksEnabled.value = enabled

  try {
    if (!encoreConfigStore.isLoaded) {
      await encoreConfigStore.loadConfig()
    }

    encoreConfigStore.setDisableTweaks(enabled)
    hasUnsavedChanges.value = true
    
    // Save config immediately
    await encoreConfigStore.saveConfig()
    hasUnsavedChanges.value = false
    
    console.log(`Disable tweaks ${enabled ? 'enabled' : 'disabled'}`)
    
    // Show reboot modal if the setting actually changed
    if (initialValue.value !== isDisableTweaksEnabled.value) {
      showRebootModal.value = true
    }
  } catch (error) {
    console.error('Failed to set disable tweaks:', error)
    isDisableTweaksEnabled.value = encoreConfigStore.isDisableTweaksEnabled
  }
}

function closeRebootModal() {
  showRebootModal.value = false
  initialValue.value = isDisableTweaksEnabled.value
}

function skipReboot() {
  closeRebootModal()
}

async function rebootDevice() {
  try {
    await exec('reboot')
  } catch (error) {
    console.error('Failed to reboot device:', error)
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
