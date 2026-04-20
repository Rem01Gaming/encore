<template>
    <Transition name="modal">
        <div v-if="show" class="fixed inset-0 z-50 flex items-center justify-center bg-black/50 p-4"
            @click="handleOutsideClick">
            <div class="bg-surface rounded-2xl w-full max-w-md max-h-[80vh] flex flex-col overflow-hidden modal-container shadow-xl mx-3"
                @click.stop>
                <div class="p-6 pb-4 border-b border-outline-variant/0">
                    <h2 class="text-2xl font-normal text-on-surface">
                        {{ title }}
                    </h2>
                    <p v-if="description" class="mt-2 text-sm text-on-surface-variant">
                        {{ description }}
                    </p>
                </div>

                <div class="overflow-y-auto px-2">
                    <slot />
                </div>

                <div class="p-6 pt-4 flex justify-end gap-2">
                    <slot name="actions" />
                </div>
            </div>
        </div>
    </Transition>
</template>

<script setup>
const props = defineProps({
    show: Boolean,
    title: String,
    description: String,
    closeOnOutsideClick: {
        type: Boolean,
        default: true
    }
})

const emit = defineEmits(['close'])

const handleOutsideClick = () => {
    if (props.closeOnOutsideClick) {
        emit('close')
    }
}
</script>

<style scoped>
.modal-enter-active,
.modal-leave-active {
    transition: opacity 150ms cubic-bezier(0.2, 0, 0, 1);
}

.modal-enter-active .modal-container,
.modal-leave-active .modal-container {
    transition: transform 150ms cubic-bezier(0.2, 0, 0, 1);
}

.modal-enter-from,
.modal-leave-to {
    opacity: 0;
}

.modal-enter-from .modal-container,
.modal-leave-to .modal-container {
    transform: scale(0.95);
}
</style>
