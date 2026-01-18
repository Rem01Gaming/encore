<template>
  <div class="flex justify-center items-center">
    <div
      class="animate-spin rounded-full border-4"
      :style="{
        width: `${size}px`,
        height: `${size}px`,
        borderColor: `color-mix(in srgb, ${color} 15%, transparent)`,
        borderTopColor: color,
      }"
    ></div>
  </div>
</template>

<script setup>
const props = defineProps({
  size: {
    type: [String, Number],
    default: 38,
  },
  color: {
    type: String,
    default: 'currentColor',
  },
})

defineOptions({
  name: 'LoadingSpinner',
})
</script>
