import './assets/main.css'

import { createApp } from 'vue'
import { createPinia } from 'pinia'
import { createI18n } from 'vue-i18n'
import { useLanguageStore } from '@/stores/Language'
import App from './App.vue'
import router from './router'

import { getPreferredLanguage, loadLocaleMessages, setI18n } from '@/helpers/Locales'

// Eruda console for debugging
if (import.meta.env.VITE_ENABLE_ERUDA === 'true') {
  import('eruda').then((eruda) => {
    eruda.default.init()
  })
}

let enMessages = {}
try {
  const enModule = await import('@/locales/strings/en.json')
  enMessages = enModule.default
} catch (error) {
  console.error('Failed to load English messages:', error)
}

const i18n = createI18n({
  legacy: false,
  locale: 'en', // Will be overridden immediately
  fallbackLocale: 'en',
  messages: {
    en: enMessages,
  },
})

// Inject i18n instance
setI18n(i18n)

async function initializeApp() {
  try {
    const { locale: preferredLocale, isUserPreference } = await getPreferredLanguage()

    const messages = await loadLocaleMessages(preferredLocale)
    i18n.global.setLocaleMessage(preferredLocale, messages)
    i18n.global.locale.value = preferredLocale

    const app = createApp(App)
    const pinia = createPinia()

    app.use(pinia)
    app.use(i18n)
    app.use(router)

    const languageStore = useLanguageStore()
    languageStore.currentLanguage = preferredLocale
    languageStore.userPreference = isUserPreference ? preferredLocale : null

    languageStore.updateHtmlAttributes()

    router.isReady().then(() => {
      app.mount('#app')
      console.log(`i18n initialized with locale: ${preferredLocale}`)
    })
  } catch (error) {
    console.error('Failed to initialize app:', error)
  }
}

initializeApp()

// Export i18n for use in other modules
export { i18n }
