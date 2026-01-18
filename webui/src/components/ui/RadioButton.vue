<template>
  <div class="inline-flex items-center group">
    <input
      :id="id"
      type="radio"
      :name="name"
      :value="value"
      :checked="modelValue === value"
      :disabled="disabled"
      @change="handleChange"
      class="absolute opacity-0 w-0 h-0 peer"
    />
    <label
      :for="id"
      class="relative inline-flex items-center cursor-pointer"
      :class="[disabled ? 'cursor-not-allowed' : 'cursor-pointer']"
    >
      <!-- Radio button container -->
      <span class="relative inline-block" :class="sizeClasses.container">
        <!-- Outer circle -->
        <span
          class="absolute rounded-full border transition-all duration-150 ease-in-out top-1/2 left-1/2 transform -translate-x-1/2 -translate-y-1/2"
          :class="[
            modelValue === value ? borderActiveClasses : borderInactiveClasses,
            sizeClasses.border,
          ]"
        ></span>

        <!-- Inner circle (dot) -->
        <span
          class="absolute rounded-full transform transition-transform duration-150 ease-in-out top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2"
          :class="[modelValue === value ? 'scale-100' : 'scale-0', sizeClasses.dot, dotColor]"
        ></span>
      </span>

      <!-- Label -->
      <span
        v-if="label"
        class="text-on-surface select-none ms-3"
        :class="disabled ? 'opacity-60' : ''"
      >
        {{ label }}
      </span>
    </label>
  </div>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  id: {
    type: String,
    default: () => `radio-${Math.random().toString(36).substr(2, 9)}`,
  },
  name: String,
  value: [String, Number, Boolean],
  modelValue: [String, Number, Boolean],
  disabled: {
    type: Boolean,
    default: false,
  },
  size: {
    type: String,
    default: 'md',
    validator: (v) => ['sm', 'md', 'lg'].includes(v),
  },
  color: {
    type: String,
    default: 'primary',
    validator: (v) => ['primary', 'secondary', 'tertiary', 'error'].includes(v),
  },
  label: {
    type: String,
    default: '',
  },
})

const emit = defineEmits(['update:modelValue'])

const handleChange = (event) => {
  if (!props.disabled) {
    emit('update:modelValue', event.target.value)
  }
}

// Size configurations
const sizeClasses = computed(() => {
  const sizes = {
    sm: {
      container: 'h-5 w-5',
      border: 'h-4 w-4',
      dot: 'h-2 w-2',
    },
    md: {
      container: 'h-6 w-6',
      border: 'h-5 w-5',
      dot: 'h-2.5 w-2.5',
    },
    lg: {
      container: 'h-7 w-7',
      border: 'h-6 w-6',
      dot: 'h-3 w-3',
    },
  }
  return sizes[props.size]
})

const borderActiveClasses = computed(() => {
  const colorMap = {
    primary: 'border-primary',
    secondary: 'border-secondary',
    tertiary: 'border-tertiary',
    error: 'border-error',
  }
  return `${colorMap[props.color]} border-2`
})

const borderInactiveClasses = computed(() => {
  return props.disabled ? 'border-outline border' : 'border-outline-variant border'
})

const dotColor = computed(() => {
  const colorMap = {
    primary: 'bg-primary',
    secondary: 'bg-secondary',
    tertiary: 'bg-tertiary',
    error: 'bg-error',
  }
  return colorMap[props.color]
})
</script>
