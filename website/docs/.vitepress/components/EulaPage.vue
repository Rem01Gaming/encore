<template>
  <div class="eula-wrapper">
    <!-- Language Switcher -->
    <div class="lang-switcher">
      <span class="lang-label">Language / Bahasa:</span>
      <div class="lang-buttons">
        <button
          :class="['lang-btn', { active: lang === 'en' }]"
          @click="setLang('en')"
        >
          English
        </button>
        <button
          :class="['lang-btn', { active: lang === 'id' }]"
          @click="setLang('id')"
        >
          Indonesia
        </button>
      </div>
    </div>

    <!-- Rendered markdown content -->
    <div v-show="lang === 'en'"><EulaEn /></div>
    <div v-show="lang === 'id'"><EulaId /></div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import EulaEn from './paid_addon/_eula-en.md'
import EulaId from './paid_addon/_eula-id.md'

const STORAGE_KEY = 'eula-lang-pref'
const lang = ref('en')

function setLang(l) {
  lang.value = l
  if (typeof localStorage !== 'undefined') {
    localStorage.setItem(STORAGE_KEY, l)
  }
}

onMounted(() => {
  if (typeof localStorage !== 'undefined') {
    const saved = localStorage.getItem(STORAGE_KEY)
    if (saved === 'en' || saved === 'id') {
      lang.value = saved
      return
    }
  }
  // Auto-detect Indonesian locale
  if ((navigator.language || '').toLowerCase().startsWith('id')) {
    lang.value = 'id'
  }
})
</script>

<style scoped>
.eula-wrapper {
  max-width: 860px;
  margin: 0 auto;
  padding: 0 1rem 3rem;
}

.lang-switcher {
  display: flex;
  align-items: center;
  gap: 12px;
  flex-wrap: wrap;
  position: sticky;
  top: var(--vp-nav-height, 64px);
  z-index: 10;
  background-color: var(--vp-c-bg);
  padding: 10px 16px;
  border-bottom: 1px solid var(--vp-c-divider);
  margin: 0 -1rem 2rem;
}

.lang-label {
  font-size: 0.875rem;
  color: var(--vp-c-text-2);
  font-weight: 500;
}

.lang-buttons {
  display: flex;
  gap: 8px;
}

.lang-btn {
  padding: 6px 16px;
  border-radius: 20px;
  border: 1px solid var(--vp-c-border);
  background: var(--vp-c-bg-soft);
  color: var(--vp-c-text-1);
  font-size: 0.875rem;
  cursor: pointer;
  transition: border-color 0.2s, color 0.2s, background-color 0.2s;
  font-family: inherit;
}

.lang-btn:hover:not(.active) {
  border-color: var(--vp-c-brand-1);
  color: var(--vp-c-brand-1);
  background: var(--vp-c-bg-soft);
}

.lang-btn.active {
    background-color: var(--vp-button-brand-bg);
    border-color: var(--vp-button-brand-border);
    color: var(--vp-button-brand-text);
    font-weight: 600;
    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
}
</style>
