import { createRouter, createWebHistory } from 'vue-router'
import { WXEventHandler } from 'webuix'

// Pages
import Home from '@/views/Home.vue'
import Games from '@/views/Games.vue'
import Settings from '@/views/Settings.vue'

// Setting pages
import LiteMode from '@/views/LiteMode.vue'
import GameSettings from '@/views/GameSettings.vue'
import LanguageSelection from '@/views/LanguageSelection.vue'
import LogLevelSelection from '@/views/LogLevelSelection.vue'
import CpuGovernor from '@/views/CpuGovernor.vue'
import DeviceMitigation from '@/views/DeviceMitigation.vue'

window.wx = new WXEventHandler()

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

// Handle back button event (WebUI X API)
wx.on(window, 'back', () => {
  const current = router.currentRoute.value.path || '/'

  if (current === '/') {
    webui.exit()
    return
  }

  // Push route /somepage/detail -> /somepage
  const segments = current.split('/').filter(Boolean)
  const parentPath = segments.length > 1 ? '/' + segments.slice(0, -1).join('/') : '/'
  router.push(parentPath)
})

export default router
