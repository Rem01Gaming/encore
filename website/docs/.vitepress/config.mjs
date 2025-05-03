import { defineConfig } from 'vitepress'

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: "Encore Tweaks",
  description: "A powerful performance magisk module featuring advanced App Monitoring, universal SoC support, and a WebUI for seamless configuration with KernelSU.",
  ignoreDeadLinks: true,

  sitemap: {
    hostname: 'https://encore.rem01gaming.dev'
  },
  
  head: [
    [
      'script',
      { async: '', src: 'https://www.googletagmanager.com/gtag/js?id=G-1962PRCX05' }
    ],
    [
      'script',
      {},
      `window.dataLayer = window.dataLayer || [];
      function gtag(){dataLayer.push(arguments);}
      gtag('js', new Date());
      gtag('config', 'G-1962PRCX05');`
    ]
  ],

  transformPageData(pageData) {
    const canonicalUrl = `${this.sitemap.hostname}/${pageData.relativePath}`
      .replace(/index\.md$/, '')
      .replace(/\.md$/, '')
    const ogImage = pageData.frontmatter.ogp ? `${this.sitemap.hostname}${pageData.frontmatter.ogp}` : `${this.sitemap.hostname}/ogp/default.webp`;
    
    pageData.frontmatter.head ??= []
    
    pageData.frontmatter.head.push(['link', { rel: "icon", type: "image/png", href: "favicon.png", sizes: "64x64"}])
    pageData.frontmatter.head.push(['link', { rel: "icon", type: "image/png", href: "android-chrome.png", sizes: "192x192"}])
    pageData.frontmatter.head.push(['link', { rel: "apple-touch-icon", type: "image/png", href: "apple-touch-icon.png", sizes: "180x180"}])
    pageData.frontmatter.head.push(['link', { rel: "canonical", href: canonicalUrl }])
    pageData.frontmatter.head.push(['meta', { name: "hostname", content: "encore.rem01gaming.dev"}])
    pageData.frontmatter.head.push(['meta', { name: "expected-hostname", content: "encore.rem01gaming.dev"}])
    pageData.frontmatter.head.push(['meta', { name: "keywords", content: "Encore Tweaks, Tweak, Magisk Module, apk, module, performance module, Gaming, Android, Module magisk, gaming performance" }])
    pageData.frontmatter.head.push(['meta', { property: "og:type", content: "website"}])
    pageData.frontmatter.head.push(['meta', { property: "og:title", content: pageData.frontmatter.layout === 'home' ? pageData.title : `${pageData.title} | Encore Tweaks` }])
    pageData.frontmatter.head.push(['meta', { property: "og:locale", content: "en_US"}])
    pageData.frontmatter.head.push(['meta', { property: "og:url", content: canonicalUrl }])
    pageData.frontmatter.head.push(['meta', { property: "og:image", content: ogImage }])
    pageData.frontmatter.head.push(['meta', { property: "og:site_name", content: "Encore Tweaks"}])
    pageData.frontmatter.head.push(['meta', { property: "og:description", content: pageData.description }])
    pageData.frontmatter.head.push(['meta', { property: "twitter:card", content: "summary_large_image"}])
    pageData.frontmatter.head.push(['meta', { property: "twitter:image", content: ogImage }])
    pageData.frontmatter.head.push(['meta', { property: "twitter:title", content: pageData.frontmatter.layout === 'home' ? pageData.title : `${pageData.title} | Encore Tweaks` }])
    pageData.frontmatter.head.push(['meta', { property: "twitter:description", content: pageData.description }])

    // Add structured data for the home page only
    if (pageData.relativePath === 'index.md') {
      pageData.frontmatter.head.push([
        'script',
        { type: "application/ld+json" },
        JSON.stringify({
          "@context": "https://schema.org",
          "@type": "WebSite",
          "name": "Encore Tweaks",
          "alternateName": "Encore Tweak",
          "url": "https://encore.rem01gaming.dev/"
        })
      ]);
    }
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
      { text: 'Download', link: '/download' }
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
        text: 'Donate',
        items: [
          { text: 'Saweria', link: 'https://saweria.co/Rem01Gaming' },
          { text: 'Buymeacoffee', link: 'https://www.buymeacoffee.com/Rem01Gaming' }
        ]
      },
      { text: 'Download', link: '/download' }
    ],
    
    footer: {
      message: 'Released under the Apache License 2.0.',
      copyright: 'Copyright © 2024-present Rem01Gaming'
    },

    socialLinks: [
      { icon: 'github', link: 'https://github.com/Rem01Gaming/encore', ariaLabel: 'GitHub' },
      { icon: 'youtube', link: 'https://youtube.com/@rem01gaming', ariaLabel: 'YouTube Channel' }
    ]
  }
})
