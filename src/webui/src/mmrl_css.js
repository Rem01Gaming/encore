const stylesheets = [
  "/mmrl/insets.css",
  "/mmrl/colors.css"
];

stylesheets.forEach((href) => {
  const link = document.createElement("link");
  link.rel = "stylesheet";
  link.type = "text/css";
  link.href = href;
  document.head.appendChild(link);
});
