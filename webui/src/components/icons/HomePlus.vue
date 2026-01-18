<template>
  <svg
    :width="size"
    :height="size"
    viewBox="0 0 24 24"
    :fill="color"
    xmlns="http://www.w3.org/2000/svg"
  >
    <path
      d="M17 14H19V17H22V19H19V22H17V19H14V17H17V14M12 3L22 12H18C14.69 12 12 14.69 12 18C12 18.7 12.12 19.37 12.34 20H5V12H2L12 3Z"
    />
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
