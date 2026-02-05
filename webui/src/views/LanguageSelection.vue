<template>
  <div class="page language-selection-page h-full flex flex-col overflow-hidden bg-surface">
    <div class="max-w-3xl mx-auto h-full flex flex-col w-full">
      <!-- Header -->
      <div class="flex-none p-5 pb-3">
        <div class="flex items-center gap-4 mb-2">
          <button @click="goBack" class="text-on-surface transition-colors">
            <ArrowLeftIcon class="w-6 h-6 cursor-pointer rtl:rotate-180" />
          </button>
          <h1 class="text-xl font-semibold text-on-surface">
            {{ $t('language_selection.title') }}
          </h1>
        </div>
      </div>

      <!-- Language List -->
      <div class="scrollbar-hidden pb-safe-nav flex-1 min-h-0 overflow-y-scroll px-5">
        <div class="space-y-0">
          <!-- Follow System Option -->
          <div
            @click="selectLanguage('system')"
            class="px-4 py-3.5 cursor-pointer transition-colors bg-transparent"
          >
            <RadioButton
              :model-value="selectedLanguage"
              value="system"
              :name="radioGroupName"
              :label="$t('language_selection.follow_system')"
              @update:model-value="selectLanguage"
            />
          </div>

          <!-- Available Languages List -->
          <div
            v-for="language in filteredAndSortedLanguages"
            :key="language.code"
            @click="selectLanguage(language.code)"
            class="px-4 py-3.5 cursor-pointer transition-colors bg-transparent"
          >
            <RadioButton
              :model-value="selectedLanguage"
              :value="language.code"
              :name="radioGroupName"
              :label="language.name"
              @update:model-value="selectLanguage"
            />
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, watch } from 'vue'
import { useRouter } from 'vue-router'
import { useLanguageStore } from '@/stores/Language'
import { detectBrowserLocale, checkLanguageFile } from '@/helpers/Locales'

import ArrowLeftIcon from '@/components/icons/ArrowLeft.vue'
import RadioButton from '@/components/ui/RadioButton.vue'

const router = useRouter()
const languageStore = useLanguageStore()

const selectedLanguage = ref('')
const radioGroupName = 'language-selection-group'

const languagesWithMissingFiles = ref([])

const filteredAndSortedLanguages = computed(() => {
  const langArray = languageStore.getAvailableLanguages()

  const filtered = langArray.filter((lang) => !languagesWithMissingFiles.value.includes(lang.code))

  const english = filtered.find((lang) => lang.code === 'en')
  const others = filtered
    .filter((lang) => lang.code !== 'en')
    .sort((a, b) => a.code.localeCompare(b.code))

  return english ? [english, ...others] : others
})

onMounted(async () => {
  const allLanguages = languageStore.getAvailableLanguages()

  const checkPromises = allLanguages.map(async (lang) => {
    const hasFile = await checkLanguageFile(lang.code)
    if (!hasFile) {
      languagesWithMissingFiles.value.push(lang.code)
    }
    return { code: lang.code, hasFile }
  })

  await Promise.all(checkPromises)

  if (languageStore.userPreference === null) {
    selectedLanguage.value = 'system'
  } else {
    if (languagesWithMissingFiles.value.includes(languageStore.userPreference)) {
      selectedLanguage.value = 'system'
      await languageStore.setLanguage(detectBrowserLocale(), false)
    } else {
      selectedLanguage.value = languageStore.userPreference
    }
  }
})

watch(
  () => languageStore.userPreference,
  (newPreference) => {
    if (newPreference === null) {
      selectedLanguage.value = 'system'
    } else if (!languagesWithMissingFiles.value.includes(newPreference)) {
      selectedLanguage.value = newPreference
    } else {
      selectedLanguage.value = 'system'
    }
  },
  { immediate: true },
)

async function selectLanguage(languageCode) {
  selectedLanguage.value = languageCode

  if (languageCode === 'system') {
    const systemLanguage = detectBrowserLocale()
    const success = await languageStore.setLanguage(systemLanguage, false)
    if (success) {
      console.log(`Language set to system default: ${systemLanguage}`)
    }
  } else {
    if (languagesWithMissingFiles.value.includes(languageCode)) {
      console.warn(`Cannot set language ${languageCode}: translation file missing`)
      return
    }

    const success = await languageStore.setLanguage(languageCode, true)
    if (success) {
      console.log(`Language set to: ${languageCode}`)
    }
  }
}

function goBack() {
  router.back()
}
</script>
