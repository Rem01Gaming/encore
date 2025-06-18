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

// Cache for English translations
let cachedEnglishTranslations = null;

async function loadTranslations(lang) {
  try {
    const response = await fetch(`locales/strings/${lang}.json`);
    const translations = await response.json();
    // Cache English translations
    if (lang === 'en') cachedEnglishTranslations = translations;
    return translations;
  } catch (error) {
    console.error('Failed to load translations:', error);
    // Return cached English if available
    if (cachedEnglishTranslations) return cachedEnglishTranslations;
    
    // Fallback to English
    const enResponse = await fetch('locales/strings/en.json');
    cachedEnglishTranslations = await enResponse.json();
    return cachedEnglishTranslations;
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
  
  // Preload English translations if not cached
  if (!cachedEnglishTranslations) {
    const enResponse = await fetch('locales/strings/en.json');
    cachedEnglishTranslations = await enResponse.json();
  }

  // Load languages list
  let languages = { en: "English" };
  try {
    const response = await fetch('locales/languages.json');
    languages = await response.json();
  } catch (error) {
    console.error('Failed to load languages list, using fallback:', error);
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
  applyTranslations(translations, cachedEnglishTranslations);
  
  // Handle language change
  selector.addEventListener('change', async (e) => {
    const newLang = e.target.value;
    localStorage.setItem('selectedLanguage', newLang);
    const newTranslations = await loadTranslations(newLang);
    applyTranslations(newTranslations, cachedEnglishTranslations);
  });
}

document.addEventListener('DOMContentLoaded', initI18n);
