/**
 * Wraps an async save function with debounce.
 *
 * @param {() => Promise<any>} saveFn - function that performs the actual save
 * @param {number} [delay=500] - debounce delay in milliseconds
 * @returns {{ trigger: () => void, flush: () => Promise<void>, cancel: () => void }}
 */
export function createDebouncedSave(saveFn, delay = 500) {
  let timeoutId = null
  let pendingPromise = null

  function queueSave(errorMessage) {
    pendingPromise = (pendingPromise ?? Promise.resolve())
      .then(() => saveFn())
      .catch((error) => {
        console.error(errorMessage, error)
      })
  }

  function trigger() {
    if (timeoutId) {
      clearTimeout(timeoutId)
    }

    timeoutId = setTimeout(() => {
      timeoutId = null
      queueSave('Debounced save failed:')
    }, delay)
  }

  async function flush() {
    if (timeoutId) {
      clearTimeout(timeoutId)
      timeoutId = null
      queueSave('Debounced save flush failed:')
    }

    if (pendingPromise) {
      await pendingPromise
    }
  }

  function cancel() {
    if (timeoutId) {
      clearTimeout(timeoutId)
      timeoutId = null
    }
  }

  return { trigger, flush, cancel }
}
