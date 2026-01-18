import { defineStore } from 'pinia'
import { i18n } from '@/main'
import languages from '@/locales/languages.json'
import { checkLanguageFile, loadLocaleMessages } from '@/helpers/Locales'

export const useLanguageStore = defineStore('language', {
  state: () => ({
    currentLanguage: i18n.global.locale,
    availableLanguages: languages,
    userPreference: null,
  }),

  getters: {
    isRTL: (state) => {
      return state.availableLanguages[state.currentLanguage]?.dir === 'rtl'
    },
    currentLanguageDir: (state) => {
      return state.availableLanguages[state.currentLanguage]?.dir || 'ltr'
    },
    languageName: (state) => (locale) => {
      return state.availableLanguages[locale]?.name || locale
    },
    displayText: (state) => {
      if (state.userPreference === null) {
        return 'Follow System'
      } else {
        const langData = state.availableLanguages[state.userPreference]
        return langData?.name || state.userPreference
      }
    },
  },

  actions: {
    async setLanguage(locale, savePreference = true) {
      try {
        if (!this.availableLanguages[locale]) {
          console.warn(`Language ${locale} is not available`)
          return false
        }

        const hasFile = await checkLanguageFile(locale)
        if (!hasFile) {
          console.warn(`Translation file not found for locale: ${locale}`)
          return false
        }

        const messages = await loadLocaleMessages(locale)

        i18n.global.setLocaleMessage(locale, messages)
        i18n.global.locale.value = locale
        this.currentLanguage = locale

        this.updateHtmlAttributes()

        if (savePreference) {
          this.userPreference = locale
          localStorage.setItem('preferred-language', locale)
        } else {
          this.userPreference = null
          localStorage.removeItem('preferred-language')
        }

        window.dispatchEvent(
          new CustomEvent('language-changed', {
            detail: { preference: this.userPreference, language: locale },
          }),
        )

        return true
      } catch (error) {
        console.error(`Failed to set language to ${locale}:`, error)
        return false
      }
    },

    initializeLanguage() {
      this.updateHtmlAttributes()
    },

    updateHtmlAttributes() {
      const html = document.documentElement
      const dir = this.currentLanguageDir
      html.setAttribute('dir', dir)
      html.setAttribute('lang', this.currentLanguage)

      document.body.classList.toggle('rtl', dir === 'rtl')
      document.body.classList.toggle('ltr', dir === 'ltr')
    },

    getAvailableLanguages() {
      return Object.entries(this.availableLanguages).map(([code, data]) => ({
        code,
        name: data.name,
        dir: data.dir,
      }))
    },
  },
})
