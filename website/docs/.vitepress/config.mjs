import { defineConfig } from 'vitepress'

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: "Encore Tweaks",
  description: "The Encore Tweaks Magisk Module is here to boost your device gaming performances but also keep battery life on normal usage",
  ignoreDeadLinks: true,

  sitemap: {
    hostname: 'https://encore.rem01gaming.dev'
  },

  transformPageData(pageData) {
    const canonicalUrl = `${this.sitemap.hostname}/${pageData.relativePath}`
      .replace(/index\.md$/, '')
      .replace(/\.md$/, '.html')
    
    pageData.frontmatter.head ??= []
    
    pageData.frontmatter.head.push(['link', { rel: "icon", type: "image/png", href: "favicon.png", sizes: "64x64"}])
    pageData.frontmatter.head.push(['link', { rel: "icon", type: "image/png", href: "android-chrome.png", sizes: "192x192"}])
    pageData.frontmatter.head.push(['link', { rel: "apple-touch-icon", type: "image/png", href: "apple-touch-icon.png", sizes: "180x180"}])
    pageData.frontmatter.head.push(['link', { rel: "canonical", href: canonicalUrl }])
    pageData.frontmatter.head.push(['meta', { name: "hostname", content: "encore.rem01gaming.dev"}])
    pageData.frontmatter.head.push(['meta', { name: "expected-hostname", content: "encore.rem01gaming.dev"}])
    pageData.frontmatter.head.push(['meta', { name: "author", content: "Rem01Gaming"}])
    pageData.frontmatter.head.push(['meta', { name: "keywords", content: "Encore Tweaks, Encore Tweak, Magisk Module, performance module, Magisk, Gaming, Android, Module magisk, gaming performance" }])
    pageData.frontmatter.head.push(['meta', { property: "og:type", content: "website"}])
    pageData.frontmatter.head.push(['meta', { property: "og:title", content: pageData.frontmatter.layout === 'home' ? pageData.title : `${pageData.title} | Encore Tweaks` }])
    pageData.frontmatter.head.push(['meta', { property: "og:locale", content: "en_US"}])
    pageData.frontmatter.head.push(['meta', { property: "og:url", content: canonicalUrl }])
    pageData.frontmatter.head.push(['meta', { property: "og:image", content: pageData.frontmatter.ogp || "/ogp/default.png" }])
    pageData.frontmatter.head.push(['meta', { property: "og:site_name", content: "Encore Tweaks"}])
    pageData.frontmatter.head.push(['meta', { property: "og:description", content: pageData.description }])
    pageData.frontmatter.head.push(['meta', { property: "twitter:card", content: "summary_large_image"}])
    pageData.frontmatter.head.push(['meta', { property: "twitter:image", content: pageData.frontmatter.ogp || "/ogp/default.png" }])
    pageData.frontmatter.head.push(['meta', { property: "twitter:title", content: pageData.frontmatter.layout === 'home' ? pageData.title : `${pageData.title} | Encore Tweaks` }])
    pageData.frontmatter.head.push(['meta', { property: "twitter:description", content: pageData.description }])
  },

  themeConfig: {
    nav: [
      { text: 'Guide', link: '/guide/what-is-encore-tweaks' },
      {
        text: 'Donate',
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
        text: 'Donate',
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
      { icon: 'youtube', link: 'https://youtube.com/@rem01gaming', ariaLabel: 'YouTube Channel' }
    ]
  }
})
