/** @type {import('tailwindcss').Config} */
module.exports = {
  content: ["./src/**/*.{html,js,ts,jsx,tsx}"],
  theme: {
    extend: {
      colors: {
        "blue-background": "#0F141B",
        "blue-secondary": "#394253",
        "blue-container": "#161D27",
        "blue-primary": "#99BBFF",
      },
    },
  },
  plugins: [require("daisyui")],
};
