/** @type {import('tailwindcss').Config} */
module.exports = {
  content: ["./src/**/*.{html,js,ts,jsx,tsx}"],
  theme: {
    extend: {
      colors: {
        "primary": "var(--primary, #99BBFF)",
        "on-primary": "var(--onPrimary, #000000)",
        "secondary": "var(--secondary, #394253)",
        "on-secondary": "var(--onSecondary, #FFFFFF)",
        "surface": "var(--surface, #0F141B)",
        "on-surface": "var(--onSurface, #FFFFFF)",
        "on-surface-variant": "var(--onSurfaceVariant, #9ca3af)",
        "surface-container": "var(--surfaceContainer, #161D27)",
      },
    },
  },
  plugins: [require("daisyui")],
};
