<template>
  <svg
    :width="size"
    :height="size"
    viewBox="0 0 24 24"
    :fill="color"
    xmlns="http://www.w3.org/2000/svg"
  >
    <path d="M21,6V8H3V6H21M3,18H12V16H3V18M3,13H21V11H3V13Z" />
  </svg>
</template>

<script setup>
const props = defineProps({
  size: {
    type: [String, Number],
    default: 24,
  },
  color: {
    type: String,
    default: 'currentColor',
  },
})
</script>
