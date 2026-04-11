import './assets/main.css'

import { createApp } from 'vue'
import { createPinia } from 'pinia'
import { createI18n } from 'vue-i18n'
import { useLanguageStore } from '@/stores/Language'
import App from './App.vue'
import router from './router'

import { openWebsite } from '@/helpers/KernelSU'
import { checkCompatibility } from '@/helpers/WebViewCompat'
import { getPreferredLanguage, loadLocaleMessages, setI18n } from '@/helpers/Locales'

// Eruda console for debugging
if (import.meta.env.VITE_ENABLE_ERUDA === 'true') {
  import('eruda').then((eruda) => {
    eruda.default.init()
  })
}

/*
 * Check WebView compatibility before initializing the app. If the WebView is too old, show a warning message instead.
 * Use inline styles for the warning message to avoid any dependency on external CSS, since the WebView might not
 * support all CSS features we use in the app.
 */
const compat = checkCompatibility()
if (!compat.ok) {
  document.body.innerHTML = `
    <div style="
      position:fixed; inset:0; z-index:99999;
      background:#1c1014; color:#f2dde2;
      font-family:sans-serif; font-size:15px;
      display:flex; flex-direction:column;
      align-items:center; justify-content:center;
      padding:24px; text-align:center; gap:16px;
    ">
      <div style="font-size:48px;">⚠️</div>
      <strong style="font-size:18px;">WebView Outdated</strong>
      <p style="color:#d5c2c6; margin:0; max-width:320px; line-height:1.5;">
        This app requires Android System WebView 121+<br>
        ${compat.missing.length ? `Missing: ${compat.missing.join(', ')}` : ''}
      </p>
      <button id="webview-update-btn" style="
        margin-top:8px;
        background:#ffb0cc; color:#541d35;
        padding:12px 24px; border-radius:24px;
        font-weight:bold; border:none; cursor:pointer;
        font-size:15px;
      ">
        Update Android System WebView
      </button>
    </div>
  `

  document.getElementById('webview-update-btn').addEventListener('click', () => {
    openWebsite('https://play.google.com/store/apps/details?id=com.google.android.webview')
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
