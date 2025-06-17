async function loadTranslations(lang) {
  try {
    const response = await fetch(`locales/strings/${lang}.json`);
    return await response.json();
  } catch (error) {
    console.error('Failed to load translations:', error);
    // Fallback to English
    const enResponse = await fetch('locales/strings/en.json');
    return await enResponse.json();
  }
}

function applyTranslations(translations) {
  document.querySelectorAll('[data-i18n]').forEach(el => {
    const keys = el.getAttribute('data-i18n').split('.');
    let value = translations;
    
    keys.forEach(key => {
      value = value?.[key];
    });
    
    if (value) el.textContent = value;
  });
}

async function initI18n() {
  const selector = document.getElementById('languageSelector');
  if (!selector) return;
  
  // Load languages list
  let languages = { en: "English" }; // Default fallback
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
  
  // Load saved or browser language with better matching
  const savedLang = localStorage.getItem('selectedLanguage');
  const browserLangs = [
    ...(navigator.languages || []),
    navigator.language,
    navigator.userLanguage
  ].filter(Boolean);
  
  let lang = savedLang;
  
  // Find best matching language
  if (!lang) {
    for (const browserLang of browserLangs) {
      // Try full language code
      if (languages[browserLang]) {
        lang = browserLang;
        break;
      }
      
      // Try base language code
      const baseLang = browserLang.split('-')[0];
      if (languages[baseLang]) {
        lang = baseLang;
        break;
      }
    }
    
    // Final fallback to English
    if (!lang) lang = 'en';
  }
  
  selector.value = lang;
  const translations = await loadTranslations(lang);
  applyTranslations(translations);
  
  // Handle language change
  selector.addEventListener('change', async (e) => {
    const newLang = e.target.value;
    localStorage.setItem('selectedLanguage', newLang);
    const newTranslations = await loadTranslations(newLang);
    applyTranslations(newTranslations);
  });
}

// Initialize on DOM load
document.addEventListener('DOMContentLoaded', initI18n);
