<template>
  <div id="app" class="copy-protected min-h-screen flex flex-col bg-background text-on-background overflow-hidden">
    <main class="main-content flex-1 md:ml-20 overflow-hidden relative">
      <router-view v-slot="{ Component, route }">
        <transition :name="transitionName">
          <component :is="Component" :key="route.path" />
        </transition>
      </router-view>
    </main>
    <Navigation />
  </div>
</template>

<script setup>
import { ref, watch } from 'vue'
import { useRoute } from 'vue-router'
import Navigation from '@/components/ui/Navigation.vue'

const route = useRoute()
const transitionName = ref('')

// Define top-level routes that should NOT animate between each other
const topLevelRoutes = ['/', '/games', '/settings']

watch(
  () => route.path,
  (to, from) => {
    // If moving between top-level pages, disable animation
    if (topLevelRoutes.includes(to) && topLevelRoutes.includes(from)) {
      transitionName.value = ''
      return
    }

    // If the 'to' path contains the 'from' path, we are going deeper (Opening Child)
    // Example: /settings -> /settings/lite_mode
    const isOpeningChild = to.startsWith(from === '/' ? '' : from) && to.length > from.length

    // If the 'from' path contains the 'to' path, we are going back (Closing Child)
    // Example: /settings/lite_mode -> /settings
    const isClosingChild = from.startsWith(to === '/' ? '' : to) && from.length > to.length

    if (isOpeningChild) {
      transitionName.value = 'page-open'
    } else if (isClosingChild) {
      transitionName.value = 'page-close'
    } else {
      transitionName.value = ''
    }
  },
)
</script>

<style>
.page-open-enter-active,
.page-open-leave-active,
.page-close-enter-active,
.page-close-leave-active {
  transition: all 150ms cubic-bezier(0.2, 0, 0, 1);
  position: absolute;
  width: 100%;
  top: var(--window-inset-top, 0px);
  bottom: 0;
  left: 0;
  will-change: transform, opacity;

  /* Force a solid background during transition to prevent 
     content from the page "underneath" from bleeding through */
  background-color: var(--color-background);
}

.page-open-enter-active {
  z-index: 2;
}

.page-open-leave-active {
  z-index: 1;
}

.page-open-leave-to {
  transform: translateX(-15%);
}

.page-open-enter-from {
  transform: translateX(15%);
  opacity: 0;
}

.page-open-enter-to {
  transform: translateX(0);
  opacity: 1;
}

.page-close-leave-active {
  z-index: 2;
}

.page-close-enter-active {
  z-index: 1;
}

.page-close-leave-from {
  transform: scale(1);
  opacity: 1;
}

.page-close-leave-to {
  transform: scale(0.95);
  opacity: 0;
}

.page-close-enter-from {
  transform: translateX(-15%);
}

.page-close-enter-to {
  transform: translateX(0);
}
</style>
