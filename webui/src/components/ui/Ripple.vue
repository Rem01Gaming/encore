<template>
  <div
    ref="container"
    class="ripple-wrapper"
    @pointerdown="handlePointerDown"
    @keydown="handleKeydown"
    :tabindex="tabindex"
    :class="{ 'non-touch': !isTouchDevice }"
    :style="hoverStyle"
  >
    <slot></slot>
  </div>
</template>

<script>
const isTouchDevice =
  typeof window !== 'undefined' &&
  ('ontouchstart' in window || navigator.maxTouchPoints > 0 || navigator.msMaxTouchPoints > 0)
</script>

<script setup>
import { ref } from 'vue'

defineProps({
  tabindex: {
    type: [String, Number],
    default: '0',
  },
})

const container = ref(null)

function handlePointerDown(event) {
  const element = container.value
  const rect = element.getBoundingClientRect()
  const size = Math.max(rect.width, rect.height)
  const x = event.clientX - rect.left - size / 2
  const y = event.clientY - rect.top - size / 2
  const duration = Math.min(0.8, Math.max(0.2, 0.2 + (rect.width / 800) * 0.3))

  const ripple = document.createElement('span')
  ripple.className = 'ripple'
  ripple.style.cssText = `
    width: ${size}px;
    height: ${size}px;
    left: ${x}px;
    top: ${y}px;
    animation-duration: ${duration}s;
    transition: opacity ${duration}s ease;
  `

  element.appendChild(ripple)

  const cleanup = () => {
    ripple.classList.add('end')
    setTimeout(() => ripple.remove(), duration * 1000)
    document.removeEventListener('pointerup', cleanup)
    document.removeEventListener('pointercancel', cleanup)
    element.removeEventListener('pointerleave', cleanup)
  }

  document.addEventListener('pointerup', cleanup)
  document.addEventListener('pointercancel', cleanup)
  element.addEventListener('pointerleave', cleanup)
}

function handleKeydown(event) {
  if (event.key === 'Enter' || event.key === ' ') {
    event.preventDefault()
    container.value.click()
  }
}

</script>

<style scoped>
.ripple-wrapper {
  position: relative;
  overflow: hidden;
  outline: none;
  transition: background-color 0.2s ease;
}

.ripple-wrapper:focus-visible {
  outline: 2px solid currentColor;
  outline-offset: 2px;
}

.ripple-wrapper.non-touch::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: transparent;
  transition: background-color 0.2s ease;
  pointer-events: none;
  border-radius: inherit;
}

.ripple-wrapper.non-touch:hover::before {
  background-color: color-mix(in srgb, currentColor 9%, transparent);
}

.ripple-wrapper.non-touch:focus-visible::before {
  z-index: -1;
}
</style>

<style>
.ripple {
  position: absolute;
  border-radius: 50%;
  transform: scale(0);
  opacity: 0.6;
  animation: ripple-animation ease-out forwards;
  pointer-events: none;
  animation-duration: 0.6s;
  background: color-mix(in srgb, currentColor 20%, transparent);
}

.ripple.end {
  opacity: 0;
}

@keyframes ripple-animation {
  to {
    transform: scale(3);
  }
}

@media (prefers-reduced-motion: reduce) {
  .ripple {
    animation-duration: 0.1s !important;
  }
  @keyframes ripple-animation {
    to {
      transform: scale(1.5);
    }
  }
}
</style>
