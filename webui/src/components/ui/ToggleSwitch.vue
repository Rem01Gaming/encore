<template>
  <label
    class="relative inline-flex cursor-pointer items-center align-middle"
    :class="{
      'opacity-38 pointer-events-none cursor-not-allowed': disabled,
    }"
  >
    <input
      :id="switchId"
      type="checkbox"
      class="peer sr-only"
      :checked="modelValue"
      @change="handleChange"
      :disabled="disabled"
    />
    <!-- Track -->
    <div
      class="relative h-8 w-13 rounded-full border-2 transition-colors duration-150 ease-in-out"
      :class="[
        modelValue ? 'border-primary bg-primary' : 'border-outline bg-surface-container-highest',
      ]"
    >
      <!-- Handle (formerly "thumb") -->
      <div
        class="absolute top-1/2 -translate-y-1/2 rounded-full shadow-sm transition-all duration-150 ease-in-out flex items-center justify-center"
        :class="[
          modelValue
            ? 'h-6 w-6 bg-on-primary rtl:-translate-x-6 ltr:translate-x-6'
            : 'h-4 w-4 bg-outline rtl:-translate-x-1.5 ltr:translate-x-1.5',
        ]"
      >
        <!-- Icon -->
        <svg
          class="h-4 w-4 transition-all duration-200"
          :class="modelValue ? 'opacity-100 scale-100' : 'opacity-0 scale-50'"
          viewBox="0 0 24 24"
          fill="currentColor"
          xmlns="http://www.w3.org/2000/svg"
        >
          <path d="M9.55 18L3.85 12.3L5.275 10.875L9.55 15.15L18.725 5.975L20.15 7.4L9.55 18Z" />
        </svg>
      </div>
    </div>
  </label>
</template>

<script setup>
const props = defineProps({
  modelValue: {
    type: Boolean,
    required: true,
  },
  disabled: {
    type: Boolean,
    default: false,
  },
  id: {
    type: String,
    default: '',
  },
})

const emit = defineEmits(['update:modelValue'])

let defaultId = 0
const switchId = props.id || `toggle-switch-${++defaultId}`

function handleChange(event) {
  const target = event.target
  emit('update:modelValue', target.checked)
}
</script>
