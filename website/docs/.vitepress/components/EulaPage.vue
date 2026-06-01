<template>
  <div class="eula-wrapper" :class="{ 'has-download': download }">

    <template v-if="!download">
      <div class="lang-switcher">
        <span class="lang-label">Language / Bahasa:</span>
        <div class="lang-buttons">
          <button :class="['lang-btn', { active: lang === 'en' }]" @click="setLang('en')">English</button>
          <button :class="['lang-btn', { active: lang === 'id' }]" @click="setLang('id')">Indonesia</button>
        </div>
      </div>
      <div v-show="lang === 'en'">
        <EulaEn />
      </div>
      <div v-show="lang === 'id'">
        <EulaId />
      </div>
    </template>

    <template v-else>
      <div v-if="status === 'loading'" class="state-card">
        <span class="spinner" aria-hidden="true"></span>
        <p class="state-msg">{{ t('loading') }}</p>
      </div>

      <div v-else-if="status === 'error'" class="state-card state-error">
        <svg width="44" height="44" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"
          stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
          <circle cx="12" cy="12" r="10" />
          <line x1="12" y1="8" x2="12" y2="12" />
          <line x1="12" y1="16" x2="12.01" y2="16" />
        </svg>
        <p class="state-msg">{{ t('invalidModule') }}</p>
      </div>

      <template v-else-if="status === 'ready'">
        <div class="lang-switcher">
          <span class="lang-label">Language / Bahasa:</span>
          <div class="lang-buttons">
            <button :class="['lang-btn', { active: lang === 'en' }]" @click="setLang('en')">English</button>
            <button :class="['lang-btn', { active: lang === 'id' }]" @click="setLang('id')">Indonesia</button>
          </div>
        </div>

        <div v-show="lang === 'en'">
          <EulaEn />
        </div>
        <div v-show="lang === 'id'">
          <EulaId />
        </div>

        <div ref="sentinelEl" class="scroll-sentinel" aria-hidden="true"></div>

        <Teleport to="body">
          <div class="dl-widget" role="region" :aria-label="t('downloadTitle', { name: moduleInfo.name })">
            <div class="dl-title-row">
              <svg class="dl-icon" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor"
                stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
                <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4" />
                <polyline points="7 10 12 15 17 10" />
                <line x1="12" y1="15" x2="12" y2="3" />
              </svg>
              <span class="dl-title">{{ t('downloadTitle', { name: moduleInfo.name }) }}</span>
            </div>

            <p class="dl-subtitle">{{ t('downloadSubtitle') }}</p>
            <div class="dl-divider" aria-hidden="true"></div>

            <div class="dl-bottom-row">
              <div class="dl-check-col">
                <p v-if="!hasScrolledToBottom" class="scroll-hint">
                  <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"
                    stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
                    <polyline points="7 13 12 18 17 13" />
                    <polyline points="7 6 12 11 17 6" />
                  </svg>
                  {{ t('scrollHint') }}
                </p>

                <label class="agree-label" :class="{ disabled: !hasScrolledToBottom }">
                  <input type="checkbox" v-model="agreed" :disabled="!hasScrolledToBottom" class="agree-checkbox" />
                  <span class="agree-text">{{ t('agreeLabel') }}</span>
                </label>
              </div>

              <button class="download-btn" :class="{ 'btn-ready': agreed }" :disabled="!agreed"
                @click="triggerDownload">
                <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"
                  stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
                  <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4" />
                  <polyline points="7 10 12 15 17 10" />
                  <line x1="12" y1="15" x2="12" y2="3" />
                </svg>
                {{ t('downloadBtn') }}
              </button>
            </div>
          </div>
        </Teleport>
      </template>
    </template>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, watch, nextTick } from 'vue'
import EulaEn from './paid_addon/_eula-en.md'
import EulaId from './paid_addon/_eula-id.md'

const props = defineProps({
  download: { type: Boolean, default: false },
})

const STRINGS = {
  en: {
    loading: 'Loading module information…',
    invalidModule: 'Invalid or missing module.',
    downloadTitle: ({ name }) => `Download ${name}`,
    downloadSubtitle: 'Before downloading, you must agree to the following terms and conditions.',
    agreeLabel: 'I have read and agree to the terms and conditions above.',
    scrollHint: 'Scroll to the bottom to enable',
    downloadBtn: 'Download',
  },
  id: {
    loading: 'Memuat informasi modul…',
    invalidModule: 'Modul tidak valid atau tidak ditemukan.',
    downloadTitle: ({ name }) => `Unduh ${name}`,
    downloadSubtitle: 'Sebelum mendownload, Anda harus menyetujui persyaratan dan ketentuan berikut.',
    agreeLabel: 'Saya telah membaca dan menyetujui persyaratan dan ketentuan di atas.',
    scrollHint: 'Gulir ke bawah untuk mengaktifkan',
    downloadBtn: 'Unduh',
  },
}

function t(key, vars = {}) {
  const entry = STRINGS[lang.value]?.[key] ?? STRINGS['en'][key]
  return typeof entry === 'function' ? entry(vars) : (entry ?? key)
}

const STORAGE_KEY = 'eula-lang-pref'
const lang = ref('en')
const status = ref('loading')
const moduleInfo = ref(null)
const hasScrolledToBottom = ref(false)
const agreed = ref(false)
const sentinelEl = ref(null)
let observer = null

function setLang(l) {
  lang.value = l
  if (typeof localStorage !== 'undefined') localStorage.setItem(STORAGE_KEY, l)
}

function initLang() {
  if (typeof localStorage === 'undefined') return
  const saved = localStorage.getItem(STORAGE_KEY)
  if (saved === 'en' || saved === 'id') { lang.value = saved; return }
  if ((navigator.language || '').toLowerCase().startsWith('id')) lang.value = 'id'
}

function setupObserver() {
  if (!sentinelEl.value) return
  observer = new IntersectionObserver(
    ([entry]) => {
      if (entry.isIntersecting) {
        hasScrolledToBottom.value = true
        observer?.disconnect()
        observer = null
      }
    },
    { threshold: 0.1 },
  )
  observer.observe(sentinelEl.value)
}

watch(lang, () => {
  if (status.value !== 'ready') return
  hasScrolledToBottom.value = false
  agreed.value = false
  observer?.disconnect()
  observer = null
  nextTick(setupObserver)
})

function getFilenameFromUrl(url) {
  const cleanUrl = url.split('?')[0].split('#')[0];
  let filename = cleanUrl.split('/').pop();
  if (!filename.toLowerCase().endsWith('.zip')) {
    filename += '.zip';
  }
  return filename;
}

function triggerDownload() {
  if (!agreed.value || !moduleInfo.value?.zipUrl) return
  const a = document.createElement('a')
  const url = moduleInfo.value.zipUrl

  a.href = url
  a.download = getFilenameFromUrl(url)
  a.rel = 'noopener noreferrer'
  document.body.appendChild(a)
  a.click()
  document.body.removeChild(a)
}

onMounted(async () => {
  initLang()
  if (!props.download) return
  const params = new URLSearchParams(window.location.search)
  const moduleId = params.get('module_id')?.trim()
  if (!moduleId) { status.value = 'error'; return }
  try {
    const res = await fetch(`https://dl.rem01gaming.dev/releases/${encodeURIComponent(moduleId)}/update.json`, { cache: 'no-cache' })
    if (!res.ok) throw new Error(`HTTP ${res.status}`)
    const data = await res.json()
    moduleInfo.value = data
    status.value = 'ready'
    await nextTick()
    setupObserver()
  } catch {
    status.value = 'error'
  }
})

onUnmounted(() => observer?.disconnect())
</script>

<style scoped>
.eula-wrapper {
  max-width: 860px;
  margin: 0 auto;
  padding: 0 1rem 3rem;
}

.eula-wrapper.has-download {
  padding-bottom: 200px;
}

.state-card {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 20px;
  min-height: 240px;
  color: var(--vp-c-text-2);
}

.state-error {
  color: var(--vp-c-danger-1, #f56060);
}

.state-msg {
  font-size: 1rem;
  font-weight: 500;
  margin: 0;
  text-align: center;
}

.spinner {
  display: block;
  width: 34px;
  height: 34px;
  border: 3px solid var(--vp-c-divider);
  border-top-color: var(--vp-c-brand-1);
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}

@keyframes spin {
  to {
    transform: rotate(360deg);
  }
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
  box-shadow: 0 -50px 0 0 var(--vp-c-bg);
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
  transition: all 0.2s;
}

.lang-btn.active {
  background-color: var(--vp-button-brand-bg);
  border-color: var(--vp-button-brand-border);
  color: var(--vp-button-brand-text);
  font-weight: 600;
}

.scroll-sentinel {
  height: 1px;
  width: 100%;
  pointer-events: none;
}

.dl-widget {
  position: fixed;
  bottom: 24px;
  left: 50%;
  transform: translateX(-50%);
  width: min(720px, calc(100vw - 48px));
  z-index: 100;
  background: var(--vp-c-bg-elv, var(--vp-c-bg-soft));
  border: 1px solid var(--vp-c-divider);
  border-radius: 16px;
  box-shadow: 0 4px 6px rgba(0, 0, 0, 0.05), 0 12px 40px rgba(0, 0, 0, 0.14);
  padding: 18px 20px 16px;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.dl-title-row {
  display: flex;
  align-items: center;
  gap: 9px;
}

.dl-icon {
  color: var(--vp-c-brand-1);
  flex-shrink: 0;
}

.dl-title {
  font-size: 1rem;
  font-weight: 700;
  color: var(--vp-c-text-1);
}

.dl-subtitle {
  font-size: 0.875rem;
  color: var(--vp-c-text-2);
  margin: 0;
}

.dl-divider {
  height: 1px;
  background: var(--vp-c-divider);
  margin: 4px 0;
}

.dl-bottom-row {
  display: flex;
  align-items: center;
  gap: 16px;
}

.dl-check-col {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.scroll-hint {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 0.875rem;
  color: var(--vp-c-text-2);
  animation: bounce 1.6s ease-in-out infinite;
}

@keyframes bounce {

  0%,
  100% {
    transform: translateY(0);
    opacity: 0.65;
  }

  50% {
    transform: translateY(3px);
    opacity: 1;
  }
}

.agree-label {
  display: flex;
  align-items: flex-start;
  gap: 9px;
  cursor: pointer;
}

.agree-label.disabled {
  opacity: 0.4;
  pointer-events: none;
}

.agree-checkbox {
  margin-top: 3px;
  accent-color: var(--vp-c-brand-1);
  width: 15px;
  height: 15px;
}

.agree-text {
  font-size: 0.875rem;
  color: var(--vp-c-text-1);
  line-height: 1.5;
}

.download-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 7px;
  width: 130px;
  padding: 10px 0;
  border-radius: 10px;
  border: 1px solid var(--vp-c-border);
  background: var(--vp-c-bg-soft);
  color: var(--vp-c-text-2);
  font-weight: 600;
  cursor: not-allowed;
  transition: all 0.22s;
}

.download-btn.btn-ready {
  background: var(--vp-button-brand-bg);
  border-color: var(--vp-button-brand-border);
  color: var(--vp-button-brand-text);
  cursor: pointer;
}

@media (max-width: 960px) {
  .lang-switcher {
    top: var(--vp-nav-height-mobile, 48px);
  }
}

@media (max-width: 560px) {
  .dl-widget {
    bottom: 16px;
    width: calc(100vw - 32px);
    padding: 16px;
  }

  .dl-bottom-row {
    flex-direction: column;
    align-items: stretch;
  }

  .download-btn {
    width: 100%;
  }
}
</style>
