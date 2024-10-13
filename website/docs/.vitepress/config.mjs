import { defineConfig } from 'vitepress'

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: "Encore Tweaks",
  description: "Special performance script for your device",
  base: '/encore/',

  sitemap: {
    hostname: 'https://rem01gaming.github.io/encore/'
  },
  
  head: [
    ['link', { rel: "icon", href: "/favicon.ico"}],
    ['link', { rel: "canonical", href: "https://rem01gaming.github.io"}],
    ['meta', { property: "og:type", content: "website"}],
    ['meta', { property: "og:url", content: "https://rem01gaming.github.io/encore/"}],
    ['meta', { property: "og:image", content: "/ogp.png"}],
    ['meta', { property: "og:site_name", content: "Encore Tweaks"}],
    ['meta', { property: "og:description", content: "Special performance script for your device"}],
    ['meta', { property: "twitter:card", content: "summary_large_image"}],
    ['meta', { property: "twitter:image", content: "/ogp.png"}],
    ['meta', { property: "twitter:title", content: "Encore Tweaks"}],
    ['meta', { property: "twitter:description", content: "Special performance script for your device"}]
  ],

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
        text: 'Download',
        items: [
          { text: 'Changelog', link: '/changelog' },
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
      },
      {
        text: 'Download',
        items: [
          { text: 'Changelog', link: '/changelog' },
          { text: 'Contribute', link: 'https://github.com/Rem01Gaming/encore' }
        ]
      },
      {
        text: 'Support my project',
        items: [
          { text: 'Saweria', link: 'https://saweria.co/Rem01Gaming' },
          { text: 'Buymeacoffee', link: 'https://www.buymeacoffee.com/Rem01Gaming' }
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
