/*
 * Copyright (C) 2024-2025 Rem01Gaming
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Cache for translations
let cachedEnglishTranslations = null;
let currentTranslations = null;

// Synchronous translation lookup
function getTranslationSync(key) {
  if (!currentTranslations || !cachedEnglishTranslations) {
    console.warn('Translations not loaded');
    return key; // Fallback to key
  }

  const keys = key.split('.');
  let value = currentTranslations;
  
  // Try current language
  for (const k of keys) {
    value = value?.[k];
    if (!value) break;
  }
  
  // Fallback to English
  if (!value) {
    value = cachedEnglishTranslations;
    for (const k of keys) {
      value = value?.[k];
      if (!value) break;
    }
  }
  
  return value || key; // Return key if no translation found
}

// Expose to global scope
window.getTranslationGlobal = getTranslationSync;

async function loadTranslations(lang) {
  if (lang === 'en' && cachedEnglishTranslations) {
    return cachedEnglishTranslations;
  }

  try {
    const response = await fetch(`locales/strings/${lang}.json`);
    const translations = await response.json();
    if (lang === 'en') cachedEnglishTranslations = translations;
    return translations;
  } catch (error) {
    console.error('Failed to load translations:', error);
    if (cachedEnglishTranslations) return cachedEnglishTranslations;
    
    try {
      const enResponse = await fetch('locales/strings/en.json');
      cachedEnglishTranslations = await enResponse.json();
      return cachedEnglishTranslations;
    } catch (fallbackError) {
      console.error('Fallback English load failed:', fallbackError);
      throw fallbackError;
    }
  }
}

function applyTranslations(translations, enTranslations) {
  document.querySelectorAll('[data-i18n]').forEach(el => {
    const keys = el.getAttribute('data-i18n').split('.');
    let value = translations;
    
    // Try to get translation from current language
    for (const key of keys) {
      value = value?.[key];
      if (value === undefined) break;
    }
    
    // Fallback to English if missing
    if (value === undefined) {
      value = enTranslations;
      for (const key of keys) {
        value = value?.[key];
        if (value === undefined) break;
      }
    }
    
    // Only update if translation found
    if (value !== undefined) el.textContent = value;
  });
}

async function initI18n() {
  const selector = document.getElementById('languageSelector');
  if (!selector) return;

  try {
    // Preload English with error handling
    if (!cachedEnglishTranslations) {
      try {
        const enResponse = await fetch('locales/strings/en.json');
        cachedEnglishTranslations = await enResponse.json();
      } catch (error) {
        console.error('Failed to preload English translations', error);
      }
    }

    // Load languages with fallback merge
    let languages = { en: "English" };
    try {
      const response = await fetch('locales/languages.json');
      const loadedLanguages = await response.json();
      languages = { ...languages, ...loadedLanguages };
    } catch (error) {
      console.error('Failed loading languages, using fallback:', error);
    }

    // Populate selector
    selector.innerHTML = '';
    for (const [code, name] of Object.entries(languages)) {
      const option = document.createElement('option');
      option.value = code;
      option.textContent = name;
      selector.appendChild(option);
    }

    // Determine language
    const savedLang = localStorage.getItem('selectedLanguage');
    const browserLangs = [
      ...(navigator.languages || []),
      navigator.language,
      navigator.userLanguage
    ].filter(Boolean);
    
    let lang = savedLang;
    if (!lang) {
      for (const browserLang of browserLangs) {
        if (languages[browserLang]) {
          lang = browserLang;
          break;
        }
        const baseLang = browserLang.split('-')[0];
        if (languages[baseLang]) {
          lang = baseLang;
          break;
        }
      }
      lang = lang || 'en';
    }
    
    selector.value = lang;
    const translations = await loadTranslations(lang);
    currentTranslations = translations; // Set current translations
    applyTranslations(translations, cachedEnglishTranslations);
    
    // Handle language change with revert-on-error
    selector.addEventListener('change', async (e) => {
      const oldLang = selector.value;
      const newLang = e.target.value;
      try {
        localStorage.setItem('selectedLanguage', newLang);
        const newTranslations = await loadTranslations(newLang);
        currentTranslations = newTranslations; // Update current translations
        applyTranslations(newTranslations, cachedEnglishTranslations);
      } catch (error) {
        selector.value = oldLang; // Revert selector
        console.error('Language switch failed:', error);
      }
    });
  } catch (error) {
    console.error('i18n initialization failed:', error);
  }
}

document.addEventListener('DOMContentLoaded', initI18n);
