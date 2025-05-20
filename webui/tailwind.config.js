/** @type {import('tailwindcss').Config} */
module.exports = {
  content: ["./src/**/*.{html,js,ts,jsx,tsx}"],
  theme: {
    extend: {
      colors: {
        /* App Base Colors */
        "primary": "var(--primary, #ffb0ccff)",
        "on-primary": "var(--onPrimary, #541d35ff)",
        "primary-container": "var(--primaryContainer, #6f334bff)",
        "on-primary-container": "var(--onPrimaryContainer, #ffd9e4ff)",
        "inverse-primary": "var(--inversePrimary, #8b4a63ff)",
        "secondary": "var(--secondary, #e2bdc8ff)",
        "on-secondary": "var(--onSecondary, #422932ff)",
        "secondary-container": "var(--secondaryContainer, #5a3f48ff)",
        "on-secondary-container": "var(--onSecondaryContainer, #ffd9e4ff)",
        "tertiary": "var(--tertiary, #f0bc95ff)",
        "on-tertiary": "var(--onTertiary, #48290dff)",
        "tertiary-container": "var(--tertiaryContainer, #623f21ff)",
        "on-tertiary-container": "var(--onTertiaryContainer, #ffdcc4ff)",
        "background": "var(--background, #1c1014ff)",
        "on-background": "var(--onBackground, #f2dde2ff)",
        "surface": "var(--surface, #1c1014ff)",
        "tonal-surface": "var(--tonalSurface, #28181dff)",
        "on-surface": "var(--onSurface, #f2dde2ff)",
        "surface-variant": "var(--surfaceVariant, #514347ff)",
        "on-surface-variant": "var(--onSurfaceVariant, #d5c2c6ff)",
        "surface-tint": "var(--surfaceTint, #ffb0ccff)",
        "inverse-surface": "var(--inverseSurface, #f2dde2ff)",
        "inverse-on-surface": "var(--inverseOnSurface, #392d31ff)",
        "error": "var(--error, #f2b8b5ff)",
        "on-error": "var(--onError, #601410ff)",
        "error-container": "var(--errorContainer, #8c1d18ff)",
        "on-error-container": "var(--onErrorContainer, #f9dedcff)",
        "outline": "var(--outline, #9d8c91ff)",
        "outline-variant": "var(--outlineVariant, #514347ff)",
        "scrim": "var(--scrim, #000000ff)",

        "surface-bright": "var(--surfaceBright, #433639ff)",
        "surface-dim": "var(--surfaceDim, #1c1014ff)",
        "surface-container": "var(--surfaceContainer, #281c20ff)",
        "surface-container-high": "var(--surfaceContainerHigh, #33272aff)",
        "surface-container-highest": "var(--surfaceContainerHighest, #3e3135ff)",
        "surface-container-low": "var(--surfaceContainerLow, #23191cff)",
        "surface-container-lowest": "var(--surfaceContainerLowest, #18090fff)",
        
        /* Filled Tonal Button Colors */
        "filled-tonal-button-content-color": "var(--filledTonalButtonContentColor, #ffd9e4ff)",
        "filled-tonal-button-container-color": "var(--filledTonalButtonContainerColor, #5a3f48ff)",
        "filled-tonal-button-disabled-content-color": "var(--filledTonalButtonDisabledContentColor, #f2dde261)",
        "filled-tonal-button-disabled-container-color": "var(--filledTonalButtonDisabledContainerColor, #f2dde21f)",
        
        /* Filled Card Colors */
        "filled-card-content-color": "var(--filledCardContentColor, #f2dde2ff)",
        "filled-card-container-color": "var(--filledCardContainerColor, #3e3135ff)",
        "filled-card-disabled-content-color": "var(--filledCardDisabledContentColor, #f2dde261)",
        "filled-card-disabled-container-color": "var(--filledCardDisabledContainerColor, #45383cff)",
      },
    },
  },
  plugins: [require("daisyui")],
};
