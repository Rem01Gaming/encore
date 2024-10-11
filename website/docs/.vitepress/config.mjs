import { defineConfig } from 'vitepress'

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: "Encore Tweaks",
  description: "Special performance script for your device",
  base: '/encore/',

  sitemap: {
    hostname: 'https://rem01gaming.github.io/encore/'
  },

  themeConfig: {
    // https://vitepress.dev/reference/default-theme-config
    nav: [
      { text: 'Guide', link: '/guide/what-is-encore-tweaks' },
      {
        text: 'Support my projects',
        items: [
          { text: 'Saweria', link: 'https://saweria.co/Rem01Gaming' },
          { text: 'Buymeacoffee', link: 'https://www.buymeacoffee.com/Rem01Gaming' }
        ]
      },
      {
        text: 'Download Encore Tweaks',
        items: [
          { text: 'Telegram Channel', link: 'https://t.me/rem01schannel' },
          { text: 'Contribute', link: 'https://github.com/Rem01Gaming/encore' }
        ]
      }
    ],

    sidebar: [
      {
        text: 'Guide',
        items: [
          { text: 'What is Encore Tweaks?', link: '/guide/what-is-encore-tweaks' },
          { text: 'Module WebUI and Configuration', link: '/guide/webui-and-configuration' },
          { text: 'FAQ', link: '/guide/faq' }
        ]
      }
    ],
    
    footer: {
      message: 'Released under the GPL3 License.',
      copyright: 'Copyright © 2024-present Rem01Gaming'
    },

    socialLinks: [
      { icon: 'github', link: 'https://github.com/Rem01Gaming', ariaLabel: 'GitHub' },
      { icon: 'youtube', link: 'https://youtube.com/@rem01gaming', ariaLabel: 'Youtube Channel' }
    ]
  }
})
