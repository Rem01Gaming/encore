import { defineConfig } from "vite"
import mkcert from "vite-plugin-mkcert";
import tailwind from "tailwindcss";
import autoprefixer from "autoprefixer";
import { ViteMinifyPlugin } from 'vite-plugin-minify';

export default defineConfig({
  root: './src',
  plugins: [
    ViteMinifyPlugin({}),
    mkcert(),
  ],
  server : {
    https: true,
  },
  css: {
    postcss: {
      plugins: [tailwind, autoprefixer],
    }
  },
  build: {
    outDir: '../dist',
  },
})
