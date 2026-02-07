<template>
    <button @click="handleClick"
        class="w-full text-left px-4 py-3 flex items-center gap-3 hover:bg-on-surface/10 transition-colors text-on-surface">
        <div v-if="$slots.icon" class="shrink-0 text-on-surface">
            <slot name="icon"></slot>
        </div>

        <span class="text-sm font-medium">
            <slot></slot>
        </span>
    </button>
</template>

<script setup>
const props = defineProps({
    action: {
        type: Function,
        default: null
    }
})

const emit = defineEmits(['click'])

function handleClick(event) {
    if (props.action) props.action()
    emit('click', event)
}
</script>