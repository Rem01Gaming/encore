import { createRouter, createWebHistory } from 'vue-router'

// Pages
import Home from '@/views/Home.vue'
import Games from '@/views/Games.vue'
import Settings from '@/views/Settings.vue'

// Setting pages
import LiteMode from '@/views/LiteMode.vue'
import DisableTweaks from '@/views/DisableTweaks.vue'
import GameSettings from '@/views/GameSettings.vue'
import LanguageSelection from '@/views/LanguageSelection.vue'
import LogLevelSelection from '@/views/LogLevelSelection.vue'
import CpuGovernor from '@/views/CpuGovernor.vue'
import DeviceMitigation from '@/views/DeviceMitigation.vue'

const routes = [
  {
    path: '/',
    name: 'Home',
    component: Home,
  },
  {
    path: '/games',
    name: 'Games',
    component: Games,
  },
  {
    path: '/settings',
    name: 'Settings',
    component: Settings,
  },

  // Setting Pages
  {
    path: '/games/:packageName',
    name: 'GameSettings',
    component: GameSettings,
  },
  {
    path: '/settings/lite_mode',
    name: 'LiteMode',
    component: LiteMode,
  },
  {
    path: '/settings/disable_tweaks',
    name: 'DisableTweaks',
    component: DisableTweaks,
  },
  {
    path: '/settings/language',
    name: 'LanguageSelection',
    component: LanguageSelection,
  },
  {
    path: '/settings/device_mitigation',
    name: 'DeviceMitigation',
    component: DeviceMitigation,
  },
  {
    path: '/settings/cpu_governor',
    name: 'CpuGovernor',
    component: CpuGovernor,
  },
  {
    path: '/settings/log_level',
    name: 'LogLevelSelection',
    component: LogLevelSelection,
  },

  // catch-all route
  {
    path: '/:pathMatch(.*)*',
    redirect: '/',
  },
]

const router = createRouter({
  history: createWebHistory(),
  routes,
})

router.beforeEach((to, from, next) => {
  next()
})

export default router
