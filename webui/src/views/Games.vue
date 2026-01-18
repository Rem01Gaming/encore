<template>
  <div class="page games-page h-full flex flex-col overflow-hidden">
    <div class="max-w-3xl mx-auto h-full flex flex-col w-full">
      <!-- Header -->
      <div class="flex-none p-5 pb-0">
        <div class="flex justify-between items-center mb-6 text-on-surface">
          <h1 class="text-xl font-semibold">{{ $t('games_page.title') }}</h1>
        </div>

        <!-- Search -->
        <div class="bg-surface-container mb-4 p-3 rounded-full">
          <div class="flex items-center gap-3">
            <SearchIcon class="ml-2 text-on-surface-variant shrink-0" />
            <input
              v-model="gamesStore.searchQuery"
              type="text"
              :placeholder="$t('games_page.search_placeholder')"
              class="bg-transparent border-none outline-none text-on-surface placeholder-on-surface-variant w-full"
            />
            <button
              v-if="gamesStore.searchQuery"
              @click="clearSearch"
              class="text-on-surface-variant hover:text-on-surface transition-colors cursor-pointer mr-3"
            >
              <CloseIcon class="w-5 h-5" />
            </button>
          </div>
        </div>
      </div>

      <!-- List -->
      <div class="scrollbar-hidden pb-26 md:pb-8 flex-1 min-h-0 overflow-y-scroll px-5">
        <LoadingSpinner class="text-primary py-8" v-if="gamesStore.isLoading" />

        <div v-else class="pb-2">
          <div
            v-for="(app, index) in gamesStore.filteredApps"
            :key="app.packageName"
            :class="['md3-list', { 'single-card-item': gamesStore.filteredApps.length === 1 }]"
          >
            <RippleComponent @click="onAppClick(app)" tabindex="0" class="md3-list-item">
              <div class="flex items-center justify-between px-5 py-4">
                <div class="flex items-center gap-4 min-w-0 flex-1">
                  <img
                    :src="app.icon"
                    loading="lazy"
                    @error="handleImageError"
                    class="w-12 h-12 rounded-full object-cover"
                    :alt="app.appName"
                  />
                  <div class="flex-1 min-w-0">
                    <h3 class="text-sm font-medium text-on-surface truncate">
                      {{ app.appName || app.packageName }}
                    </h3>
                    <!-- Only show package name if it's different from app name -->
                    <p
                      v-if="app.appName && app.appName !== app.packageName"
                      class="text-xs text-on-surface-variant truncate mt-1"
                    >
                      {{ app.packageName }}
                    </p>
                    <div v-if="app.isEnabled" class="flex items-center gap-1 mt-1">
                      <span class="inline-flex items-center bg-primary rounded-sm px-1.5 py-0.5">
                        <span class="text-[10px] text-on-primary font-semibold uppercase">{{
                          $t('games_page.badges.tweak_enabled')
                        }}</span>
                      </span>
                    </div>
                  </div>
                </div>

                <div
                  class="w-7 h-7 rounded-full bg-surface-dim flex items-center justify-center shrink-0 ml-3"
                >
                  <ChevronRightIcon
                    class="text-on-surface-variant shrink-0 rtl:rotate-180"
                    :size="22"
                  />
                </div>
              </div>
            </RippleComponent>
          </div>

          <div
            v-if="gamesStore.filteredApps.length === 0 && !gamesStore.isLoading"
            class="text-center py-8 text-on-surface-variant"
          >
            <p>{{ $t('games_page.no_apps_found') }}</p>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useGamesStore } from '@/stores/Games'

import LoadingSpinner from '@/components/ui/LoadingSpinner.vue'
import RippleComponent from '@/components/ui/Ripple.vue'
import SearchIcon from '@/components/icons/Search.vue'
import CloseIcon from '@/components/icons/Close.vue'
import ChevronRightIcon from '@/components/icons/ChevronRight.vue'

const router = useRouter()
const gamesStore = useGamesStore()

const initialLoadComplete = ref(false)

onMounted(async () => {
  if (gamesStore.userApps.length === 0) {
    gamesStore.isLoading = true
    await gamesStore.initializeData()
    initialLoadComplete.value = true
  } else {
    initialLoadComplete.value = true
  }
})

const clearSearch = () => {
  gamesStore.searchQuery = ''
}

const onAppClick = (app) => {
  router.push(`/games/${app.packageName}`)
}

const handleImageError = (e) => {
  e.target.src = '/app_icon_fallback.avif'
}
</script>
