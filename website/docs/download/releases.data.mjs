import { createContentLoader } from "vitepress";

export default createContentLoader("download/version/*.md", {
  // 'render: true' tells VitePress to parse the markdown into raw HTML
  render: true,
  transform(rawData) {
    return rawData.sort((a, b) => {
      const verA = a.frontmatter.version || "0";
      const verB = b.frontmatter.version || "0";

      // localeCompare with numeric: true natively handles semantic versioning (e.g., 5.10 > 5.2)
      return verB.localeCompare(verA, undefined, {
        numeric: true,
        sensitivity: "base",
      });
    });
  },
});
