<template>
  <div class="page h-full flex flex-col overflow-hidden bg-surface">
    <div class="max-w-3xl mx-auto h-full flex flex-col w-full">
      <div class="flex-none p-5 pb-3">
        <div class="flex items-center gap-4 mb-2">
          <button @click="goBack" class="text-on-surface rounded-full">
            <ArrowLeftIcon class="w-6 h-6 cursor-pointer rtl:rotate-180" />
          </button>
        </div>
      </div>

      <div class="scrollbar-hidden pb-25 md:pb-4 flex-1 min-h-0 overflow-y-scroll px-5">
        <div class="space-y-6">
          <h1 class="text-4xl text-on-surface mt-12 mb-6">
            {{ $t('cpu_governor.title') }}
          </h1>

          <div class="bg-surface-container-highest rounded-2xl p-4">
            <h3 class="text-base font-medium text-on-surface mb-1">
              {{ $t('cpu_governor.default_title') }}
            </h3>
            <p class="text-sm text-on-surface-variant mb-4">
              {{ $t('cpu_governor.default_description') }}
            </p>
            <Ripple class="rounded-xl">
              <button
                @click="openModal('default')"
                class="w-full bg-surface-container-low rounded-xl px-4 py-3 text-left flex items-center justify-between hover:bg-surface-container"
              >
                <span class="text-sm text-on-surface">{{ balanceGovernor || 'schedutil' }}</span>
                <ChevronRightIcon class="w-5 h-5 text-on-surface-variant" />
              </button>
            </Ripple>
          </div>

          <div class="bg-surface-container-highest rounded-2xl p-4">
            <h3 class="text-base font-medium text-on-surface mb-1">
              {{ $t('cpu_governor.powersave_title') }}
            </h3>
            <p class="text-sm text-on-surface-variant mb-4">
              {{ $t('cpu_governor.powersave_description') }}
            </p>
            <Ripple class="rounded-xl">
              <button
                @click="openModal('powersave')"
                class="w-full bg-surface-container-low rounded-xl px-4 py-3 text-left flex items-center justify-between hover:bg-surface-container"
              >
                <span class="text-sm text-on-surface">{{ powersaveGovernor || 'schedutil' }}</span>
                <ChevronRightIcon class="w-5 h-5 text-on-surface-variant" />
              </button>
            </Ripple>
          </div>

          <InformationOutlineIcon class="text-on-surface-variant my-6" :size="22" />
          <p class="text-sm text-on-surface-variant leading-relaxed">
            {{ $t('cpu_governor.brief') }}
          </p>
        </div>
      </div>
    </div>

    <Transition name="modal">
      <div
        v-if="showModal"
        class="fixed inset-0 z-50 flex items-center justify-center bg-black/50 p-4"
        @click="closeModal"
      >
        <div
          class="bg-surface rounded-2xl w-full max-w-md max-h-[80vh] overflow-hidden modal-container shadow-xl mx-3"
          @click.stop
        >
          <div class="p-5 border-b border-outline-variant">
            <h2 class="text-lg font-medium text-on-surface">
              {{ modalTitle }}
            </h2>
          </div>

          <div class="overflow-y-auto max-h-[60vh]">
            <div class="p-2">
              <div
                v-for="governor in availableGovernors"
                :key="governor"
                @click="selectAndApplyGovernor(governor)"
                class="px-4 py-3 flex items-center gap-3 hover:bg-surface-container rounded-lg cursor-pointer"
              >
                <RadioButton
                  :model-value="selectedGovernor"
                  :value="governor"
                  :label="governor"
                  size="md"
                  class="flex-1"
                />
              </div>
            </div>
          </div>

          <div class="p-4 border-t border-outline-variant flex justify-end">
            <Ripple class="rounded-full">
              <button
                @click="closeModal"
                class="px-4 py-2 text-sm font-medium text-primary hover:text-on-surface"
              >
                {{ $t('common.cancel') }}
              </button>
            </Ripple>
          </div>
        </div>
      </div>
    </Transition>
  </div>
</template>

<script setup>
import { ref, onMounted, computed } from 'vue'
import { useRouter } from 'vue-router'
import { useI18n } from 'vue-i18n'
import { useEncoreConfigStore } from '@/stores/EncoreConfig'
import * as KernelSU from '@/helpers/KernelSU'

import ArrowLeftIcon from '@/components/icons/ArrowLeft.vue'
import ChevronRightIcon from '@/components/icons/ChevronRight.vue'
import RadioButton from '@/components/ui/RadioButton.vue'
import InformationOutlineIcon from '@/components/icons/InformationOutline.vue'
import Ripple from '@/components/ui/Ripple.vue'

const router = useRouter()
const encoreConfigStore = useEncoreConfigStore()
const { t } = useI18n()

const availableGovernors = ref([])
const showModal = ref(false)
const modalType = ref('default') // 'default' or 'powersave'
const selectedGovernor = ref('')
const hasUnsavedChanges = ref(false)

const balanceGovernor = computed(() => encoreConfigStore.balanceGovernor)
const powersaveGovernor = computed(() => encoreConfigStore.powersaveGovernor)

const modalTitle = computed(() => {
  return modalType.value === 'default'
    ? t('cpu_governor.modal.default_select_title')
    : t('cpu_governor.modal.powersave_select_title')
})

onMounted(async () => {
  try {
    if (!encoreConfigStore.isLoaded) {
      await encoreConfigStore.loadConfig()
    }
    await loadAvailableGovernors()
  } catch (error) {
    console.error('Failed to initialize CPU governor page:', error)
  }
})

async function loadAvailableGovernors() {
  try {
    const governorsFile = '/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors'

    if (await KernelSU.fileExists(governorsFile)) {
      const content = await KernelSU.readFile(governorsFile)
      availableGovernors.value = content
        .trim()
        .split(/\s+/)
        .filter((g) => g)
    } else {
      availableGovernors.value = []
    }
  } catch (error) {
    console.error('Failed to load available governors:', error)
    availableGovernors.value = []
  }
}

function openModal(type) {
  modalType.value = type
  selectedGovernor.value = type === 'default' ? balanceGovernor.value : powersaveGovernor.value
  showModal.value = true
}

function closeModal() {
  showModal.value = false
}

async function selectAndApplyGovernor(governor) {
  try {
    if (modalType.value === 'default') {
      encoreConfigStore.setBalanceGovernor(governor)
    } else {
      encoreConfigStore.setPowersaveGovernor(governor)
    }

    hasUnsavedChanges.value = true
    await encoreConfigStore.saveConfig()
    closeModal()
  } catch (error) {
    console.error('Failed to apply governor:', error)
  }
}

function goBack() {
  if (hasUnsavedChanges.value) {
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
  } else {
    router.back()
  }
}
</script>

<style scoped>
.modal-enter-active,
.modal-leave-active {
  transition: opacity 150ms cubic-bezier(0, 0, 0.2, 1);
}

.modal-enter-active .modal-container,
.modal-leave-active .modal-container {
  transition: transform 150ms cubic-bezier(0, 0, 0.2, 1);
}

.modal-enter-from,
.modal-leave-to {
  opacity: 0;
}

.modal-enter-from .modal-container,
.modal-leave-to .modal-container {
  transform: scale(0.92);
}
</style>
