const REQUIRED_FEATURES = [
  {
    name: 'dvh units',
    test: () => CSS.supports('height', '100dvh'),
  },
  {
    name: '@layer',
    test: () => typeof CSSLayerStatementRule !== 'undefined',
  },
  {
    name: 'CSS custom props',
    test: () => CSS.supports('color', 'var(--test)'),
  },
  {
    name: 'overscroll-behavior',
    test: () => CSS.supports('overscroll-behavior', 'none'),
  },
  {
    name: 'scrollbar-width',
    test: () => {
      const el = document.createElement('div')
      el.style.scrollbarWidth = 'none'
      return el.style.scrollbarWidth === 'none'
    },
  },
  {
    name: 'ES modules',
    test: () => 'noModule' in document.createElement('script'),
  },
]

export function getMissingFeatures() {
  return REQUIRED_FEATURES.filter((f) => {
    try {
      return !f.test()
    } catch {
      return true
    }
  }).map((f) => f.name)
}

export function checkCompatibility() {
  const missing = getMissingFeatures()

  return {
    ok: missing.length === 0,
    missing,
  }
}
