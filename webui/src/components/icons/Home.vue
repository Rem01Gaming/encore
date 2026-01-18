<template>
  <svg
    xmlns="http://www.w3.org/2000/svg"
    :width="size"
    :height="size"
    viewBox="0 -960 960 960"
    :fill="color"
  >
    <path v-if="active" d="M160-120v-480l320-240 320 240v480H560v-280H400v280H160Z" />
    <path
      v-else
      d="M240-200h120v-240h240v240h120v-360L480-740 240-560v360Zm-80 80v-480l320-240 320 240v480H520v-240h-80v240H160Zm320-350Z"
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
  active: {
    type: Boolean,
    default: false,
  },
})
</script>
