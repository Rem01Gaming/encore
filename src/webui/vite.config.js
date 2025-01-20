import { defineConfig } from "vite"
import tailwind from "tailwindcss";
import autoprefixer from "autoprefixer";

export default defineConfig({
  root: './src',
  css: {
    postcss: {
      plugins: [tailwind, autoprefixer],
    }
  },
  build: {
    outDir: '../dist',
  },
})
