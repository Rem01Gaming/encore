import { defineConfig } from 'vitepress'

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: "Encore Tweaks",
  description: "Encore Tweaks is a free and open source performance Magisk module designed to boost device performance while playing games but keeping battery life on normal usage",
  base: '/encore/',

  sitemap: {
    hostname: 'https://rem01gaming.github.io/encore/'
  },
  
  head: [
    ['link', { rel: "icon", type: "image/png", href: "/encore/favicon.png", size: "64x64"}],
    ['link', { rel: "icon", type: "image/png", href: "/encore/android-crhome.png", size: "192x192"}],
    ['link', { rel: "apple-touch-icon", type: "image/png", href: "/encore/apple-touch-icon.png", size: "180x180"}],
    ['link', { rel: "canonical", href: "https://rem01gaming.github.io/encore/"}],
    ['meta', { property: "og:type", content: "website"}],
    ['meta', { property: "og:url", content: "https://rem01gaming.github.io/encore/"}],
    ['meta', { property: "og:image", content: "/encore/ogp.png"}],
    ['meta', { property: "og:site_name", content: "Encore Tweaks"}],
    ['meta', { property: "og:description", content: "Special performance module for your device"}],
    ['meta', { property: "twitter:card", content: "summary_large_image"}],
    ['meta', { property: "twitter:image", content: "/encore/ogp.png"}],
    ['meta', { property: "twitter:title", content: "Encore Tweaks"}],
    ['meta', { property: "twitter:description", content: "Special performance module for your device"}]
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
          { text: 'Changelog', link: '/download' },
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
          { text: 'Changelog', link: '/download' },
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
