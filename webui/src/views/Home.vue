<template>
  <div class="page home-page h-full flex flex-col">
    <!-- Header -->
    <div class="sticky top-0 z-10 bg-background">
      <div class="max-w-3xl mx-auto p-5 pb-3">
        <div class="flex justify-between items-center text-on-surface">
          <h1 class="text-xl font-semibold">{{ $t('home_page.title') }}</h1>
        </div>
      </div>
    </div>

    <!-- Scrollable Content -->
    <div class="scrollbar-hidden pb-15 md:pb-0 flex-1 min-h-0 overflow-y-scroll">
      <div class="max-w-3xl mx-auto p-5 pt-1">
        <!-- Daemon Status -->
        <div
          class="bg-secondary-container mb-4 p-4 rounded-xl flex items-center justify-between text-on-secondary-container"
        >
          <img
            :src="homeStore.logoImage"
            class="mx-2 w-22 h-22"
            alt="Encore Logo"
            rel="preload"
            tabindex="0"
          />
          <div class="flex-1 flex flex-col px-3">
            <span class="text-lg font-semibold">{{ daemonStatusText }}</span>
            <span class="text-xs pt-1 block">{{ daemonPidText }}</span>
          </div>
        </div>

        <!-- Device & Module Info -->
        <div class="bg-surface-container mb-4 p-4 rounded-xl text-on-surface">
          <div class="py-2 px-2 flex items-start gap-4">
            <StarIcon class="text-primary mt-2 shrink-0" />
            <div>
              <h3 class="text-sm font-medium text-on-surface">
                {{ $t('home_page.info_card.module') }}
              </h3>
              <span class="allow-copy text-xs text-on-surface-variant block mt-1">{{
                displayValue(homeStore.moduleVersion)
              }}</span>
            </div>
          </div>

          <div class="py-2 px-2 flex items-start gap-4">
            <StarlyGear class="text-primary mt-2 shrink-0" />
            <div>
              <h3 class="text-sm font-medium text-on-surface">
                {{ $t('home_page.info_card.profile') }}
              </h3>
              <span class="allow-copy text-xs text-on-surface-variant block mt-1">{{
                currentProfileText
              }}</span>
            </div>
          </div>

          <div class="py-2 px-2 flex items-start gap-4">
            <ConsoleIcon class="text-primary mt-2 shrink-0" />
            <div>
              <h3 class="text-sm font-medium text-on-surface">
                {{ $t('home_page.info_card.kernel') }}
              </h3>
              <span class="allow-copy text-xs text-on-surface-variant block mt-1">{{
                displayValue(homeStore.kernelVersion)
              }}</span>
            </div>
          </div>

          <div class="py-2 px-2 flex items-start gap-4">
            <ChipsetIcon class="text-primary mt-2 shrink-0" />
            <div>
              <h3 class="text-sm font-medium text-on-surface">
                {{ $t('home_page.info_card.chipset') }}
              </h3>
              <span class="allow-copy text-xs text-on-surface-variant block mt-1">{{
                displayValue(homeStore.chipsetName)
              }}</span>
            </div>
          </div>

          <div class="py-2 px-2 flex items-start gap-4">
            <AndroidIcon class="text-primary mt-2 shrink-0" />
            <div>
              <h3 class="text-sm font-medium text-on-surface">
                {{ $t('home_page.info_card.androidSDK') }}
              </h3>
              <span class="allow-copy text-xs text-on-surface-variant block mt-1">{{
                displayValue(homeStore.androidSDK)
              }}</span>
            </div>
          </div>
        </div>

        <!-- Support Me Button -->
        <RippleComponent
          @click="handleDonateClick"
          tabindex="0"
          class="cursor-pointer text-on-surface bg-surface-container mb-4 p-4 py-5 rounded-xl w-full"
        >
          <h2 class="text-sm font-medium px-2 mb-1 relative z-10">
            {{ $t('home_page.support_button.title') }}
          </h2>
          <p class="text-sm text-on-surface-variant px-2 mb-1 relative z-10">
            {{ $t('home_page.support_button.description') }}
          </p>
        </RippleComponent>

        <!-- Learn Encore -->
        <RippleComponent
          @click="handleGuideClick"
          tabindex="0"
          class="cursor-pointer text-on-surface bg-surface-container mb-4 p-4 py-5 rounded-xl w-full"
        >
          <h2 class="text-sm font-medium px-2 mb-1 relative z-10">
            {{ $t('home_page.learn_encore.title') }}
          </h2>
          <p class="text-sm text-on-surface-variant px-2 mb-1 relative z-10">
            {{ $t('home_page.learn_encore.description') }}
          </p>
        </RippleComponent>
      </div>
    </div>
  </div>
</template>

<script setup>
import { onMounted, onUnmounted, computed } from 'vue'
import { useHomeStore } from '@/stores/Home'
import * as KernelSU from '@/helpers/KernelSU'
import { useI18n } from 'vue-i18n'

import RippleComponent from '@/components/ui/Ripple.vue'
import StarIcon from '@/components/icons/Star.vue'
import StarlyGear from '@/components/icons/StarlyGear.vue'
import ConsoleIcon from '@/components/icons/Console.vue'
import ChipsetIcon from '@/components/icons/Chipset.vue'
import AndroidIcon from '@/components/icons/Android.vue'

const { t } = useI18n()
const homeStore = useHomeStore()

// Helper function to display values with proper i18n
function displayValue(value) {
  if (value === 'unknown' || !value) {
    return t('common.unknown')
  }
  return value
}

// Computed properties for translated text
const daemonStatusText = computed(() => {
  const status = homeStore.daemonStatusRaw
  if (status === 'loading') return t('common.loading')
  return t(`home_page.status_card.${status}`)
})

const daemonPidText = computed(() => {
  const status = homeStore.daemonStatusRaw
  if (status === 'running' && homeStore.daemonPidRaw) {
    return t('home_page.status_card.daemonPID', { pid: homeStore.daemonPidRaw })
  } else if (status === 'stopped') {
    return t('home_page.status_card.daemon_inactive')
  } else if (status === 'error' && homeStore.daemonError) {
    return homeStore.daemonError
  }
  return t('home_page.status_card.loading_daemon')
})

const currentProfileText = computed(() => {
  const profileKey = homeStore.currentProfileRaw
  if (profileKey === 'unknown' || !profileKey) return t('common.unknown')

  // Check if translation exists, fallback to raw key
  const translation = t(`profiles.${profileKey}`)
  return translation !== `profiles.${profileKey}` ? translation : profileKey
})

onMounted(async () => {
  await homeStore.initializeData()
})

onUnmounted(() => {
  homeStore.stopProfileMonitoring()
})

function handleGuideClick() {
  KernelSU.openWebsite('https://encore.rem01gaming.dev/guide/what-is-encore-tweaks')
}

function handleDonateClick() {
  KernelSU.openWebsite('https://t.me/rem01schannel/670')
}
</script>
