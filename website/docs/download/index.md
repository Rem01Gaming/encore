---
title: "Download"
description: "Download the latest version of Encore Tweaks Magisk Module here"
---

<script setup>
import { data as releases } from './releases.data.mjs'
const latest = releases[0]
const olderReleases = releases.slice(1)
</script>

<div v-if="latest" v-html="latest.html" class="latest-release-container"></div>

---

## Download Older Versions

<ul>
  <li v-for="release in olderReleases" :key="release.url">
    <a :href="release.url">Version {{ release.frontmatter.version }}</a>
  </li>
</ul>
