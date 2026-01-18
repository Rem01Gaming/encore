import languages from '@/locales/languages.json'

let i18nInstance = null

/**
 * Sets the i18n instance to be used outside of Vue components
 * @param {Object} i18n - The Vue I18n instance
 */
export function setI18n(i18n) {
  i18nInstance = i18n
}

/**
 * Gets a translation for a given key outside of the setup script
 * @param {string} key - The translation key
 * @param {Object} [params] - Named parameters for the translation
 * @returns {string} The translated string
 */
export function getTranslation(key, params) {
  if (!i18nInstance) {
    console.warn('[Locales] i18n instance has not been set yet.')
    return key
  }

  return i18nInstance.global.t(key, params)
}

/**
 * Gets the preferred language based on user preference or browser detection
 */
export async function getPreferredLanguage() {
  // First check for user's saved preference
  const savedLanguage = localStorage.getItem('preferred-language')

  if (savedLanguage && languages[savedLanguage]) {
    const hasTranslationFile = await checkLanguageFile(savedLanguage)
    if (hasTranslationFile) {
      return { locale: savedLanguage, isUserPreference: true }
    }
  }

  // If no saved preference or file missing, detect from browser
  const detectedLocale = detectBrowserLocale()
  const hasTranslationFile = await checkLanguageFile(detectedLocale)

  if (hasTranslationFile) {
    return { locale: detectedLocale, isUserPreference: false }
  }

  // Fallback to English if detected language file doesn't exist
  return { locale: 'en', isUserPreference: false }
}

/**
 * Detects browser locale with fallback logic
 */
export function detectBrowserLocale() {
  const browserLang = navigator.language || navigator.userLanguage || 'en'

  // Direct match
  if (languages[browserLang]) {
    return browserLang
  }

  // Try without region code (e.g., en-US -> en)
  const langWithoutRegion = browserLang.split('-')[0]
  if (languages[langWithoutRegion]) {
    return langWithoutRegion
  }

  // Check if any available language starts with the browser language
  for (const availableLang in languages) {
    if (availableLang.startsWith(langWithoutRegion)) {
      return availableLang
    }
  }

  return 'en'
}

/**
 * Checks if a translation file exists for a given locale
 */
export async function checkLanguageFile(locale) {
  try {
    await import(`@/locales/strings/${locale}.json`)
    return true
  } catch (error) {
    console.warn(`Translation file not found for locale: ${locale}`, error)
    return false
  }
}

/**
 * Loads locale messages for a given locale
 */
export async function loadLocaleMessages(locale) {
  try {
    const messages = await import(`@/locales/strings/${locale}.json`)
    return messages.default
  } catch (error) {
    console.warn(`Failed to load locale ${locale}, falling back to English`)
    if (locale !== 'en') {
      const enMessages = await import(`@/locales/strings/en.json`)
      return enMessages.default
    }
    throw error
  }
}
