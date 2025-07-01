import { defineConfig } from "vite"
import mkcert from "vite-plugin-mkcert";
import tailwind from "tailwindcss";
import autoprefixer from "autoprefixer";
import Terminal from 'vite-plugin-terminal';
import { ViteMinifyPlugin } from 'vite-plugin-minify';

export default defineConfig({
  root: './src',
  plugins: [
    ViteMinifyPlugin({}),
    mkcert(),
    Terminal({
      console: 'terminal',
      output: ['terminal', 'console'],
    }),
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
