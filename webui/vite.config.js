import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
//import vueDevTools from 'vite-plugin-vue-devtools'
import tailwindcss from '@tailwindcss/vite'
import mkcert from "vite-plugin-mkcert";
import { ViteMinifyPlugin } from 'vite-plugin-minify';

// https://vite.dev/config/
export default defineConfig({
  plugins: [
    vue(),
    //vueDevTools(),
    tailwindcss(),
    ViteMinifyPlugin({}),
    mkcert(),
  ],
  server : {
    https: true,
  },
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    },
  },
})
