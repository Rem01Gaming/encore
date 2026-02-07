<template>
    <div class="relative">
        <div @click="toggle">
            <slot name="trigger" :is-open="isOpen"></slot>
        </div>

        <div v-if="isOpen" @click="close" class="fixed inset-0 z-40"></div>

        <Transition enter-active-class="transition duration-150 ease-out-quart"
            enter-from-class="transform scale-90 opacity-0" enter-to-class="transform scale-100 opacity-100"
            leave-active-class="transition duration-100 ease-in" leave-from-class="transform scale-100 opacity-100"
            leave-to-class="transform scale-90 opacity-0">
            <div v-if="isOpen"
                class="absolute ltr:right-0 rtl:left-0 top-full mt-2 w-max min-w-35 rounded-md bg-surface-container-high shadow-xl border border-outline-variant/20 overflow-hidden z-50 origin-top-right py-2">
                <slot name="content" :close="close"></slot>
            </div>
        </Transition>
    </div>
</template>

<script setup>
import { ref } from 'vue'

const isOpen = ref(false)

function toggle() {
    isOpen.value = !isOpen.value
}

function close() {
    isOpen.value = false
}

defineExpose({ open: () => isOpen.value = true, close, toggle })
</script>