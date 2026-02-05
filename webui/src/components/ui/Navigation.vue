<template>
  <nav
    class="footer fixed bottom-0 left-0 right-0 w-full flex items-end bg-surface-container shadow-lg z-50 md:left-0 md:top-0 md:bottom-0 md:w-23 md:h-full md:flex-col backdrop-blur-md"
    :style="{
      paddingBottom: 'var(--window-inset-bottom, 0px)',
      paddingRight: 'var(--window-inset-right, 0px)',
      paddingLeft: 'var(--window-inset-left, 0px)'
    }">
    <div class="w-full h-20 flex items-center justify-center md:h-full md:flex-col md:justify-center">
      <router-link v-for="item in navItems" :key="item.name" :to="item.path"
        class="footer-btn gap-1 w-full max-w-50 text-on-secondary-container border-none bg-transparent text-sm flex justify-center items-center flex-col user-select-none p-0 no-underline transition-all duration-200 md:max-h-min md:py-3"
        :class="{
          'text-on-background': isActive(item),
          'text-on-surface-variant': !isActive(item),
        }">
        <div
          class="footer-btn-icon h-8 flex justify-center items-center rounded-full transition-all duration-200 ease-in-out"
          :class="{
            'bg-secondary-container px-5': isActive(item),
            'px-0': !isActive(item),
          }">
          <component :is="item.icon" :active="isActive(item)" />
        </div>
        <div class="footer-btn-text text-xs">
          <span class="font-medium">{{ item.label }}</span>
        </div>
      </router-link>
    </div>
  </nav>
</template>

<script setup>
import { computed } from 'vue'
import { useRoute } from 'vue-router'
import { useI18n } from 'vue-i18n'

import HomeIcon from '@/components/icons/Home.vue'
import GamesIcon from '@/components/icons/Games.vue'
import SettingsIcon from '@/components/icons/Settings.vue'

const { t } = useI18n()
const route = useRoute()

const navItems = computed(() => [
  {
    name: 'Home',
    path: '/',
    label: t('navigation.home'),
    icon: HomeIcon,
  },
  {
    name: 'Games',
    path: '/games',
    label: t('navigation.games'),
    icon: GamesIcon,
  },
  {
    name: 'Settings',
    path: '/settings',
    label: t('navigation.settings'),
    icon: SettingsIcon,
  },
])

const isActive = (item) => {
  const currentPath = route.path
  if (item.path === '/') return currentPath === '/'
  return currentPath.startsWith(item.path)
}
</script>

<style scoped>
.footer-btn {
  flex: 1;
}

.footer-btn-icon {
  transition:
    background-color 0.2s ease,
    padding 0.25s cubic-bezier(0.4, 0, 0.2, 1);
}
</style>
