import { defineConfig } from "vite"
import tailwind from "tailwindcss";
import autoprefixer from "autoprefixer";
import { ViteMinifyPlugin } from 'vite-plugin-minify'

export default defineConfig({
  root: './src',
  plugins: [
    ViteMinifyPlugin({}),
  ],
  css: {
    postcss: {
      plugins: [tailwind, autoprefixer],
    }
  },
  build: {
    outDir: '../dist',
  },
})
