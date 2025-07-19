import DefaultTheme from 'vitepress/theme'
import { h } from 'vue'
import './custom.css'
import MyLayout from "./components/Layout.vue";


export default {
  extends: DefaultTheme,
  Layout: MyLayout,
}