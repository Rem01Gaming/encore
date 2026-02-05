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

      <div class="scrollbar-hidden pb-safe-nav flex-1 min-h-0 overflow-y-scroll px-5">
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
              <button @click="openSelectionModal('default')"
                class="w-full bg-surface-container-low rounded-xl px-4 py-3 text-left flex items-center justify-between hover:bg-surface-container">
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
              <button @click="openSelectionModal('powersave')"
                class="w-full bg-surface-container-low rounded-xl px-4 py-3 text-left flex items-center justify-between hover:bg-surface-container">
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

    <Modal :show="showSelectionModal" :title="modalTitle" @close="closeSelectionModal">
      <div class="p-2 border-y border-outline-variant">
        <div v-for="governor in availableGovernors" :key="governor" @click="handleGovernorSelect(governor)"
          class="px-4 py-3 flex items-center gap-3 hover:bg-surface-container rounded-lg cursor-pointer">
          <RadioButton :model-value="selectedGovernor" :value="governor" :label="governor" size="md"
            class="flex-1 pointer-events-none" />
        </div>
      </div>
      <template #actions>
        <button @click="closeSelectionModal"
          class="px-4 py-2 text-sm font-medium text-primary hover:bg-primary/10 rounded-full transition-colors">
          {{ $t('common.cancel') }}
        </button>
      </template>
    </Modal>

    <Modal :show="showWarningModal" :title="$t('common.warning')" @close="cancelPerformanceSelection">
      <div class="px-4 pb-2">
        <p class="text-sm">
          {{ $t('cpu_governor.modal.performance_warning') }}
        </p>
      </div>

      <template #actions>
        <button @click="cancelPerformanceSelection"
          class="px-4 py-2 text-sm font-medium text-primary hover:bg-primary/10 rounded-full">
          {{ $t('common.cancel') }}
        </button>
        <button @click="confirmPerformanceSelection"
          class="px-4 py-2 text-sm font-medium text-error hover:bg-error/10 rounded-full">
          {{ $t('cpu_governor.modal.performance_warning_confirm') }}
        </button>
      </template>
    </Modal>
  </div>
</template>

<script setup>
import { ref, onMounted, computed } from 'vue'
import { useRouter } from 'vue-router'
import { useI18n } from 'vue-i18n'
import { useEncoreConfigStore } from '@/stores/EncoreConfig'
import * as KernelSU from '@/helpers/KernelSU'

// Components
import ArrowLeftIcon from '@/components/icons/ArrowLeft.vue'
import ChevronRightIcon from '@/components/icons/ChevronRight.vue'
import RadioButton from '@/components/ui/RadioButton.vue'
import InformationOutlineIcon from '@/components/icons/InformationOutline.vue'
import Ripple from '@/components/ui/Ripple.vue'
import Modal from '@/components/ui/Modal.vue'

const router = useRouter()
const encoreConfigStore = useEncoreConfigStore()
const { t } = useI18n()

// State
const availableGovernors = ref([])
const showSelectionModal = ref(false)
const showWarningModal = ref(false)
const modalType = ref('default') // 'default' or 'powersave'
const selectedGovernor = ref('')
const pendingGovernor = ref('') // Stores choice while warning is shown
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

function openSelectionModal(type) {
  modalType.value = type
  selectedGovernor.value = type === 'default' ? balanceGovernor.value : powersaveGovernor.value
  showSelectionModal.value = true
}

function closeSelectionModal() {
  showSelectionModal.value = false
}

function handleGovernorSelect(governor) {
  if (governor === 'performance') {
    pendingGovernor.value = governor
    showWarningModal.value = true
    showSelectionModal.value = false
  } else {
    applyGovernor(governor)
  }
}

function cancelPerformanceSelection() {
  showWarningModal.value = false
  pendingGovernor.value = ''
  showSelectionModal.value = true
}

function confirmPerformanceSelection() {
  if (pendingGovernor.value) {
    applyGovernor(pendingGovernor.value)
    pendingGovernor.value = ''
  }
  showWarningModal.value = false
}

async function applyGovernor(governor) {
  try {
    if (modalType.value === 'default') {
      encoreConfigStore.setBalanceGovernor(governor)
    } else {
      encoreConfigStore.setPowersaveGovernor(governor)
    }

    hasUnsavedChanges.value = true
    await encoreConfigStore.saveConfig()
    closeSelectionModal()
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