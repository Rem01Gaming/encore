import { defineConfig } from "vitepress";

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: "Encore Tweaks",
  lang: "en-US",
  cleanUrls: true,

  sitemap: {
    hostname: "https://encore.rem01gaming.dev",
  },

  head: [
    ["link", { rel: "icon", type: "image/x-icon", href: "/favicon.ico" }],
    ["meta", { name: "hostname", content: "encore.rem01gaming.dev" }],
    ["meta", { name: "expected-hostname", content: "encore.rem01gaming.dev" }],
    ["meta", { property: "og:type", content: "website" }],
    ["meta", { property: "og:locale", content: "en-US" }],
    ["meta", { property: "og:site_name", content: "Encore Tweaks" }],
    ["meta", { property: "twitter:card", content: "summary_large_image" }],

    // Google Analytics
    [
      "script",
      {
        async: "",
        src: "https://www.googletagmanager.com/gtag/js?id=G-1962PRCX05",
      },
    ],
    [
      "script",
      {},
      `window.dataLayer = window.dataLayer || [];
      function gtag(){dataLayer.push(arguments);}
      gtag('js', new Date());
      gtag('config', 'G-1962PRCX05');`,
    ],
  ],

  transformPageData(pageData) {
    const canonicalUrl = `${this.sitemap.hostname}/${pageData.relativePath}`
      .replace(/index\.md$/, "")
      .replace(/\.md$/, "");
    const ogImage = pageData.frontmatter.ogp
      ? `${this.sitemap.hostname}${pageData.frontmatter.ogp}?v=2`
      : `${this.sitemap.hostname}/ogp/default.webp?v=2`;

    pageData.frontmatter.head ??= [];

    // Add dynamic meta tags
    pageData.frontmatter.head.push(
      ["link", { rel: "canonical", href: canonicalUrl }],
      [
        "meta",
        {
          property: "og:title",
          content:
            pageData.frontmatter.layout === "home"
              ? pageData.title
              : `${pageData.title} | Encore Tweaks`,
        },
      ],
      ["meta", { property: "og:url", content: canonicalUrl }],
      ["meta", { property: "og:image", content: ogImage }],
      ["meta", { property: "og:description", content: pageData.description }],
      ["meta", { property: "twitter:image", content: ogImage }],
      [
        "meta",
        {
          property: "twitter:title",
          content:
            pageData.frontmatter.layout === "home"
              ? pageData.title
              : `${pageData.title} | Encore Tweaks`,
        },
      ],
      [
        "meta",
        { property: "twitter:description", content: pageData.description },
      ],
    );

    // Structured data (homepage only)
    if (pageData.relativePath === "index.md") {
      pageData.frontmatter.head.push([
        "script",
        { type: "application/ld+json" },
        JSON.stringify({
          "@context": "https://schema.org",
          "@type": "SoftwareApplication",
          name: "Encore Tweaks",
          operatingSystem: "Android",
          applicationCategory: "UtilitiesApplication",
          offers: {
            "@type": "Offer",
            price: "0",
            priceCurrency: "USD",
          },
          url: "https://encore.rem01gaming.dev/",
        }),
      ]);
    }
  },

  themeConfig: {
    nav: [
      { text: "Guide", link: "/guide/" },
      { text: "Donate", link: "/download/donate" },
      { text: "Download", link: "/download" },
      { text: "Legal", link: "/legal/" },
    ],

    sidebar: {
      "/guide/": [
        {
          text: "Guide",
          items: [
            { text: "What is Encore Tweaks?", link: "/guide/" },
            { text: "Getting Started", link: "/guide/getting-started" },
            { text: "Lite Mode", link: "/guide/lite-mode" },
            { text: "Encore Tweaks Addon", link: "/guide/addon" },
            { text: "FAQ", link: "/guide/faq" },
          ],
        },
        { text: "Download", link: "/download/" },
        { text: "Donate", link: "/download/donate" },
      ],
      "/download": [
        { text: "Download", link: "/download/" },
        { text: "Donate", link: "/download/donate" },
      ],
      "/legal/": [
        {
          text: "Legal",
          items: [
            { text: "Apache License 2.0", link: "/legal/" },
            { text: "Third Party Notices", link: "/legal/third-party" },
            { text: "Paid Addon EULA", link: "/legal/paid-addon-eula" },
          ],
        },
      ],
    },

    search: {
      provider: "local",
      options: {
        locales: {
          root: {
            translations: {
              button: {
                buttonText: "Search Documents",
                buttonAriaLabel: "Search Documents",
              },
              modal: {
                noResultsText: "No results could be found",
                resetButtonTitle: "Clear the search criteria",
                footer: {
                  selectText: "option",
                  navigateText: "switchover",
                },
              },
            },
          },
        },
      },
    },

    footer: {
      message: "Released under the Apache License 2.0.",
      copyright: "Copyright &copy 2024-present Rem01Gaming",
    },

    editLink: {
      pattern:
        "https://github.com/Rem01Gaming/encore/edit/main/website/docs/:path",
      text: "Edit this page in GitHub",
    },

    socialLinks: [
      {
        icon: "github",
        link: "https://github.com/Rem01Gaming/encore",
        ariaLabel: "GitHub",
      },
    ],
  },
});
