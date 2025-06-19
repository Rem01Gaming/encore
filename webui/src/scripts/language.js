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

// Import core translations statically
import enTranslations from '../locales/strings/en.json';
import languages from '../locales/languages.json';

// Cache for translations
const cachedEnglishTranslations = enTranslations;
let currentTranslations = null;

// Dynamic imports for non-English translations
const translationModules = import.meta.glob(
  '../locales/strings/!(en).json',  // Exclude en.json from dynamic imports
  { eager: false }
);

// Synchronous translation lookup
// This function also will parse args from it's input
function getTranslationSync(key, ...args) {
  if (!currentTranslations) {
    console.error('Translations not loaded!');
    return key;
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

  // Return key if no translation found
  if (!value) return key;

  // Handle placeholder replacement
  if (args.length > 0 && typeof value === 'string') {
    return value.replace(/\{(\d+)\}/g, (match, index) => {
      const idx = parseInt(index);
      return args[idx] !== undefined ? args[idx] : match;
    });
  }

  return value;
}

// Expose to global scope
window.getTranslation = getTranslationSync;

async function loadTranslations(lang) {
  // Use static import for English
  if (lang === 'en') return cachedEnglishTranslations;

  const filePath = `../locales/strings/${lang}.json`;
  
  if (translationModules[filePath]) {
    try {
      const module = await translationModules[filePath]();
      return module.default;
    } catch (error) {
      console.error(`Failed to load ${lang} translations:`, error);
      return cachedEnglishTranslations;
    }
  } else {
    console.warn(`No translation file for ${lang}, falling back to English`);
    return cachedEnglishTranslations;
  }
}

function applyTranslations(translations) {
  document.querySelectorAll('[data-i18n]').forEach(el => {
    const keys = el.getAttribute('data-i18n').split('.');
    let value = translations;
    
    for (const key of keys) {
      value = value?.[key];
      if (value === undefined) break;
    }
    
    if (value !== undefined) {
      el.textContent = value;
    }
  });
}

async function initI18n() {
  const selector = document.getElementById('languageSelector');
  if (!selector) return;

  try {
    // Merge languages with English as default
    const allLanguages = { en: "English", ...languages };

    // Populate selector
    selector.innerHTML = '';
    for (const [code, name] of Object.entries(allLanguages)) {
      const option = document.createElement('option');
      option.value = code;
      option.textContent = name;
      selector.appendChild(option);
    }

    // Determine initial language
    const savedLang = localStorage.getItem('selectedLanguage');
    const browserLangs = [
      ...(navigator.languages || []),
      navigator.language,
      navigator.userLanguage
    ].filter(Boolean);
    
    let lang = savedLang || 'en';
    if (!savedLang) {
      for (const browserLang of browserLangs) {
        const normalizedLang = browserLang.toLowerCase().replace(/_/g, '-');
        if (allLanguages[normalizedLang]) {
          lang = normalizedLang;
          break;
        }
        const baseLang = normalizedLang.split('-')[0];
        if (allLanguages[baseLang]) {
          lang = baseLang;
          break;
        }
      }
    }
    
    selector.value = lang;
    currentTranslations = await loadTranslations(lang);
    applyTranslations(currentTranslations);
    
    // Handle language change
    selector.addEventListener('change', async (e) => {
      const newLang = e.target.value;
      const oldTranslations = currentTranslations;
      
      try {
        currentTranslations = await loadTranslations(newLang);
        applyTranslations(currentTranslations);
        localStorage.setItem('selectedLanguage', newLang);
      } catch (error) {
        currentTranslations = oldTranslations;
        selector.value = lang;
        console.error('Language switch failed:', error);
      }
      
      lang = newLang;
    });
  } catch (error) {
    console.error('i18n initialization failed:', error);
  }
}

// Initialize immediately if DOM is ready, otherwise wait
if (document.readyState !== 'loading') {
  initI18n();
} else {
  document.addEventListener('DOMContentLoaded', initI18n);
}
