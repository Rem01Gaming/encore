<template>
  <div class="page settings-page h-full flex flex-col overflow-hidden">
    <div class="sticky top-0 z-10 bg-background">
      <div class="max-w-3xl mx-auto p-5 pb-3">
        <div class="flex justify-between items-center text-on-surface">
          <h1 class="text-xl font-semibold">{{ $t('settings_page.title') }}</h1>
        </div>
      </div>
    </div>

    <div class="scrollbar-hidden pb-safe-nav flex-1 min-h-0 overflow-y-scroll">
      <div class="max-w-3xl mx-auto p-5 py-1">
        <div class="px-4 py-2 mb-1">
          <h2 class="text-sm font-medium text-on-surface-variant">
            {{ $t('settings_page.section.preferences') }}
          </h2>
        </div>

        <div class="space-y-1.5 mb-4">
          <div class="md3-list">
            <RippleComponent @click="openLiteModeView" class="md3-list-item" tabindex="0">
              <div class="flex items-center justify-between px-5 py-4">
                <div class="flex items-center gap-4 min-w-0 flex-1">
                  <div class="w-10 h-10 rounded-full bg-primary-container flex items-center justify-center shrink-0">
                    <FeatherIcon class="w-5 h-5 text-on-primary-container" />
                  </div>

                  <div class="flex-1 min-w-0">
                    <h3 class="text-sm font-medium text-on-surface">
                      {{ $t('settings_page.lite_mode.title') }}
                    </h3>
                    <p class="text-xs text-on-surface-variant mt-1 line-clamp-2">
                      {{ $t('settings_page.lite_mode.description') }}
                    </p>
                  </div>
                </div>

                <div class="w-7 h-7 rounded-full bg-surface-dim flex items-center justify-center shrink-0 ml-3">
                  <ChevronRightIcon class="text-on-surface-variant shrink-0 rtl:rotate-180" :size="22" />
                </div>
              </div>
            </RippleComponent>
          </div>

          <div class="md3-list">
            <RippleComponent @click="openLanguageView" class="md3-list-item" tabindex="0">
              <div class="flex items-center justify-between px-5 py-4">
                <div class="flex items-center gap-4 min-w-0 flex-1">
                  <div class="w-10 h-10 rounded-full bg-primary-container flex items-center justify-center shrink-0">
                    <LanguageIcon class="w-5 h-5 text-on-primary-container" />
                  </div>

                  <div class="flex-1 min-w-0">
                    <h3 class="text-sm font-medium text-on-surface">
                      {{ $t('settings_page.language.title') }}
                    </h3>
                    <p class="text-xs text-on-surface-variant mt-1">
                      {{ currentLanguage }}
                    </p>
                  </div>
                </div>

                <div class="w-7 h-7 rounded-full bg-surface-dim flex items-center justify-center shrink-0 ml-3">
                  <ChevronRightIcon class="text-on-surface-variant shrink-0 rtl:rotate-180" :size="22" />
                </div>
              </div>
            </RippleComponent>
          </div>
        </div>

        <div class="px-4 py-2 mb-1">
          <h2 class="text-sm font-medium text-on-surface-variant">
            {{ $t('settings_page.section.system') }}
          </h2>
        </div>

        <div class="space-y-1.5 mb-4">
          <div class="md3-list">
            <RippleComponent @click="openDeviceMitigationView" class="md3-list-item" tabindex="0">
              <div class="flex items-center justify-between px-5 py-4">
                <div class="flex items-center gap-4 min-w-0 flex-1">
                  <div class="w-10 h-10 rounded-full bg-primary-container flex items-center justify-center shrink-0">
                    <BugIcon class="w-5 h-5 text-on-primary-container" />
                  </div>

                  <div class="flex-1 min-w-0">
                    <h3 class="text-sm font-medium text-on-surface">
                      {{ $t('settings_page.device_mitigation.title') }}
                    </h3>
                    <p class="text-xs text-on-surface-variant mt-1 line-clamp-2">
                      {{ $t('settings_page.device_mitigation.description') }}
                    </p>
                  </div>
                </div>

                <div class="w-7 h-7 rounded-full bg-surface-dim flex items-center justify-center shrink-0 ml-3">
                  <ChevronRightIcon class="text-on-surface-variant shrink-0 rtl:rotate-180" :size="22" />
                </div>
              </div>
            </RippleComponent>
          </div>

          <div class="md3-list">
            <RippleComponent @click="openCpuGovernorView" class="md3-list-item" tabindex="0">
              <div class="flex items-center justify-between px-5 py-4">
                <div class="flex items-center gap-4 min-w-0 flex-1">
                  <div class="w-10 h-10 rounded-full bg-primary-container flex items-center justify-center shrink-0">
                    <ChipsetIcon class="w-5 h-5 text-on-primary-container" />
                  </div>
                  <div class="flex-1 min-w-0">
                    <h3 class="text-sm font-medium text-on-surface">
                      {{ $t('settings_page.cpu_governor.title') }}
                    </h3>
                    <p class="text-xs text-on-surface-variant mt-1 line-clamp-2">
                      {{ $t('settings_page.cpu_governor.description') }}
                    </p>
                  </div>
                </div>

                <div class="w-7 h-7 rounded-full bg-surface-dim flex items-center justify-center shrink-0 ml-3">
                  <ChevronRightIcon class="text-on-surface-variant shrink-0 rtl:rotate-180" :size="22" />
                </div>
              </div>
            </RippleComponent>
          </div>

          <div class="md3-list">
            <RippleComponent @click="openLogLvlView" class="md3-list-item" tabindex="0">
              <div class="flex items-center justify-between px-5 py-4">
                <div class="flex items-center gap-4 min-w-0 flex-1">
                  <div class="w-10 h-10 rounded-full bg-primary-container flex items-center justify-center shrink-0">
                    <TextIcon class="w-5 h-5 text-on-primary-container" />
                  </div>

                  <div class="flex-1 min-w-0">
                    <h3 class="text-sm font-medium text-on-surface">
                      {{ $t('settings_page.log_level.title') }}
                    </h3>
                    <p class="text-xs text-on-surface-variant mt-1 line-clamp-2">
                      {{ $t('settings_page.log_level.description') }}
                    </p>
                  </div>
                </div>

                <div class="w-7 h-7 rounded-full bg-surface-dim flex items-center justify-center shrink-0 ml-3">
                  <ChevronRightIcon class="text-on-surface-variant shrink-0 rtl:rotate-180" :size="22" />
                </div>
              </div>
            </RippleComponent>
          </div>
        </div>

        <div class="px-4 py-2 mb-1">
          <h2 class="text-sm font-medium text-on-surface-variant">
            {{ $t('settings_page.section.others') }}
          </h2>
        </div>

        <div class="space-y-1.5 mb-4">
          <div class="md3-list">
            <RippleComponent @click="openExportModal" class="md3-list-item" tabindex="0">
              <div class="flex items-center justify-between px-5 py-4">
                <div class="flex items-center gap-4 min-w-0 flex-1">
                  <div class="w-10 h-10 rounded-full bg-primary-container flex items-center justify-center shrink-0">
                    <ContentSaveIcon class="w-5 h-5 text-on-primary-container" />
                  </div>

                  <div class="flex-1 min-w-0">
                    <h3 class="text-sm font-medium text-on-surface">
                      {{ $t('settings_page.save_log.title') }}
                    </h3>
                    <p class="text-xs text-on-surface-variant mt-1 line-clamp-2">
                      {{ $t('settings_page.save_log.description') }}
                    </p>
                  </div>
                </div>

                <div class="w-7 h-7 rounded-full bg-surface-dim flex items-center justify-center shrink-0 ml-3">
                  <ChevronRightIcon class="text-on-surface-variant shrink-0 rtl:rotate-180" :size="22" />
                </div>
              </div>
            </RippleComponent>
          </div>

          <div class="md3-list">
            <RippleComponent @click="createShortcut" class="md3-list-item" tabindex="0">
              <div class="flex items-center justify-between px-5 py-4">
                <div class="flex items-center gap-4 min-w-0 flex-1">
                  <div class="w-10 h-10 rounded-full bg-primary-container flex items-center justify-center shrink-0">
                    <HomePlusIcon class="w-5 h-5 text-on-primary-container" />
                  </div>

                  <div class="flex-1 min-w-0">
                    <h3 class="text-sm font-medium text-on-surface">
                      {{ $t('settings_page.create_shortcut.title') }}
                    </h3>
                    <p class="text-xs text-on-surface-variant mt-1 line-clamp-2">
                      {{ $t('settings_page.create_shortcut.description') }}
                    </p>
                  </div>
                </div>

                <div class="w-7 h-7 rounded-full bg-surface-dim flex items-center justify-center shrink-0 ml-3">
                  <ChevronRightIcon class="text-on-surface-variant shrink-0 rtl:rotate-180" :size="22" />
                </div>
              </div>
            </RippleComponent>
          </div>
        </div>
      </div>
    </div>

    <Modal :show="showExportModal" :title="$t('settings_page.save_log.title')" @close="closeExportModal"
      :closeOnOutsideClick="false">
      <div class="px-4 pb-2">
        <div v-if="exportStatus === 'loading'" class="flex flex-col items-center gap-4 py-6">
          <LoadingSpinner :size="40" class="text-primary" />
          <p class="text-on-surface-variant text-sm">{{ $t('settings_page.save_log.exporting') }}</p>
        </div>

        <div v-else-if="exportStatus === 'success'" class="flex flex-col items-center gap-3 py-4">
          <CheckCircle :size="48" class="text-primary" />
          <p class="text-on-surface font-medium text-center">{{ $t('settings_page.save_log.success') }}</p>
          <p
            class="text-on-surface-variant text-xs break-all text-center bg-surface-container-low px-4 py-3 rounded-xl w-full">
            {{ exportPath }}
          </p>
        </div>

        <div v-else-if="exportStatus === 'error'" class="flex flex-col items-center gap-3 py-4">
          <ErrorIcon :size="48" class="text-error" />
          <p class="text-on-surface font-medium text-center">{{ $t('settings_page.save_log.failure') }}</p>
          <p class="text-on-surface-variant text-sm text-center">{{ exportErrorMsg }}</p>
        </div>
      </div>

      <template #actions>
        <div v-if="exportStatus !== 'loading'" class="flex gap-2">
          <button @click="closeExportModal"
            class="px-4 py-2 text-sm font-medium text-primary hover:bg-primary/10 rounded-full transition-colors">
            {{ exportStatus === 'success' ? 'Close' : $t('common.ok') }}
          </button>
        </div>
      </template>
    </Modal>
  </div>
</template>

<script setup>
import { ref, computed } from 'vue'
import { useRouter } from 'vue-router'
import { useI18n } from 'vue-i18n'
import { useLanguageStore } from '@/stores/Language'

import RippleComponent from '@/components/ui/Ripple.vue'
import ChevronRightIcon from '@/components/icons/ChevronRight.vue'
import LanguageIcon from '@/components/icons/Language.vue'
import FeatherIcon from '@/components/icons/Feather.vue'
import ChipsetIcon from '@/components/icons/Chipset.vue'
import BugIcon from '@/components/icons/Bug.vue'
import TextIcon from '@/components/icons/Text.vue'
import HomePlusIcon from '@/components/icons/HomePlus.vue'
import ContentSaveIcon from '@/components/icons/ContentSave.vue'
import ErrorIcon from '@/components/icons/Error.vue'
import Modal from '@/components/ui/Modal.vue'
import LoadingSpinner from '@/components/ui/LoadingSpinner.vue'
import CheckCircle from '@/components/icons/CheckCircle.vue'

import * as KernelSU from '@/helpers/KernelSU'
import { exec } from 'kernelsu'

const router = useRouter()
const { t } = useI18n()
const languageStore = useLanguageStore()

// Modal state
const showExportModal = ref(false)
const exportStatus = ref('idle') // 'idle', 'loading', 'success', 'error'
const exportPath = ref('')
const exportErrorMsg = ref('')

// Computed property for current language display
const currentLanguage = computed(() => {
  if (languageStore.userPreference === null) {
    return t('language_selection.follow_system')
  } else {
    const langCode = languageStore.userPreference
    const langData = languageStore.availableLanguages[langCode]
    return langData?.name || langCode
  }
})

const openLiteModeView = () => router.push('/settings/lite_mode')
const openLanguageView = () => router.push('/settings/language')
const openDeviceMitigationView = () => router.push('/settings/device_mitigation')
const openCpuGovernorView = () => router.push('/settings/cpu_governor')
const openLogLvlView = () => router.push('/settings/log_level')
const createShortcut = () => KernelSU.createShortcut()

const openExportModal = () => {
  exportStatus.value = 'loading'
  exportPath.value = ''
  exportErrorMsg.value = ''
  showExportModal.value = true

  setTimeout(() => {
    exec(`/data/adb/modules/encore/system/bin/encore_utility save_logs`)
      .then(({ errno, stdout, stderr }) => {
        if (errno !== 0) {
          exportStatus.value = 'error'
          exportErrorMsg.value = stderr.trim()
        } else {
          exportStatus.value = 'success'
          exportPath.value = stdout.trim()
        }
      })
      .catch((err) => {
        console.error('Export error:', err)
        exportStatus.value = 'error'
        exportErrorMsg.value = err.message
      })
  }, 100)
}

const closeExportModal = () => {
  showExportModal.value = false
  setTimeout(() => {
    if (exportStatus.value !== 'loading') {
      exportStatus.value = 'idle'
      exportPath.value = ''
      exportErrorMsg.value = ''
    }
  }, 200)
}
</script>

<style scoped>
.line-clamp-2 {
  display: -webkit-box;
  line-clamp: 2;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
}
</style>
