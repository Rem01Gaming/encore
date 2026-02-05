<template>
  <div class="page log-level-selection-page h-full flex flex-col overflow-hidden bg-surface">
    <div class="max-w-3xl mx-auto h-full flex flex-col w-full">
      <!-- Header -->
      <div class="flex-none p-5 pb-3">
        <div class="flex items-center gap-4 mb-2">
          <button @click="goBack" class="text-on-surface transition-colors">
            <ArrowLeftIcon class="w-6 h-6 cursor-pointer rtl:rotate-180" />
          </button>
          <h1 class="text-xl font-semibold text-on-surface">
            {{ $t('log_level_selection.title') }}
          </h1>
        </div>
      </div>

      <!-- Log Level List -->
      <div class="scrollbar-hidden pb-safe-nav flex-1 min-h-0 overflow-y-scroll px-5">
        <div class="space-y-0">
          <!-- Log Level Options -->
          <div
            v-for="level in logLevels"
            :key="level.value"
            @click="selectLogLevel(level.value)"
            class="px-4 py-3.5 cursor-pointer transition-colors bg-transparent"
          >
            <RadioButton
              :model-value="selectedLevel"
              :value="level.value"
              :name="radioGroupName"
              :label="level.label"
              @update:model-value="selectLogLevel"
            />
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useEncoreConfigStore } from '@/stores/EncoreConfig'
import { useI18n } from 'vue-i18n'

import ArrowLeftIcon from '@/components/icons/ArrowLeft.vue'
import RadioButton from '@/components/ui/RadioButton.vue'

const router = useRouter()
const encoreConfigStore = useEncoreConfigStore()
const { t } = useI18n()

const selectedLevel = ref(3)
const radioGroupName = 'log-level-selection-group'

const logLevels = [
  {
    value: 0,
    label: t('log_level_selection.level_0'),
  },
  {
    value: 1,
    label: t('log_level_selection.level_1'),
  },
  {
    value: 2,
    label: t('log_level_selection.level_2'),
  },
  {
    value: 3,
    label: t('log_level_selection.level_3'),
  },
  {
    value: 4,
    label: t('log_level_selection.level_4'),
  },
  {
    value: 5,
    label: t('log_level_selection.level_5'),
  },
]

onMounted(async () => {
  try {
    if (!encoreConfigStore.isLoaded) {
      await encoreConfigStore.loadConfig()
    }

    selectedLevel.value = encoreConfigStore.logLevel
  } catch (error) {
    console.error('Failed to load log level:', error)
  }
})

async function selectLogLevel(level) {
  selectedLevel.value = level

  try {
    encoreConfigStore.setLogLevel(level)
    await encoreConfigStore.saveConfig()
    console.log(`Log level set to: ${level}`)
  } catch (error) {
    console.error('Failed to save log level:', error)
  }
}

function goBack() {
  router.back()
}
</script>
