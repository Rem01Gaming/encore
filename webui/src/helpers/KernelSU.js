import { wrapInputStream, Intent, WebUI } from 'webuix'
import { exec, toast } from 'kernelsu'
import { getTranslation } from '@/helpers/Locales'
import { moduleInterface, fileInterface, packageManagerInterface } from './WXInterfaces'

/**
 * Check if running on KernelSU WebUI
 * @returns {boolean}
 */
export function isKSUWebUI() {
  return typeof ksu !== 'undefined'
}

/**
 * Check if running on WebUI X
 * @returns {boolean}
 */
export function isRunningOnWebUIX() {
  return typeof $encore !== 'undefined' && Object.keys($encore).length > 0
}

/**
 * Create WebUI shortcut
 */
export function createShortcut() {
  if (!isRunningOnWebUIX()) {
    const shortcut_unavailable = getTranslation("toast.shortcut_unavailable");
    toast(shortcut_unavailable)
    return
  }

  if (moduleInterface.hasShortcut()) {
    const has_shortcut = getTranslation("toast.has_shortcut");
    toast(has_shortcut)
    return
  }

  moduleInterface.createShortcut()
}

/**
 * Read a file from the filesystem
 * @param {string} filePath - Path to the file
 * @returns {Promise<string>} Trimmed file contents
 * @throws {Error} If not running on KSU WebUI or file cannot be read
 */
export async function readFile(filePath) {
  if (!isKSUWebUI()) {
    throw new Error('Not running on KSU WebUI')
  }

  if (fileInterface) {
    if (!fileInterface.exists(filePath)) {
      throw new Error(`File cannot be read: ${filePath}`)
    }

    return fileInterface.read(filePath).trim()
  }

  const { errno, stdout, stderr } = await exec(`[ -f "${filePath}" ] && cat "${filePath}"`)

  if (errno != 0) {
    throw new Error(`File cannot be read: ${stderr}`)
  }

  return stdout.trim()
}

/**
 * Write content to a file
 * @param {string} filePath - Path to the file
 * @param {string} content - Content to write
 * @returns {Promise<void>}
 * @throws {Error} If not running on KSU WebUI
 */
export async function writeFile(filePath, content) {
  if (!isKSUWebUI()) {
    throw new Error('Not running on KSU WebUI')
  }

  if (fileInterface) {
    fileInterface.write(filePath, content)
  } else {
    const escapedContent = content.replace(/'/g, "'\\''")
    await exec(`echo '${escapedContent}' > "${filePath}"`)
  }
}

/**
 * Check if a file exists
 * @param {string} filePath - Path to the file
 * @returns {Promise<boolean>}
 * @throws {Error} If not running on KSU WebUI
 */
export async function fileExists(filePath) {
  if (!isKSUWebUI()) {
    throw new Error('Not running on KSU WebUI')
  }

  if (fileInterface) {
    return fileInterface.exists(filePath)
  } else {
    const { errno } = await exec(`[ -f "${filePath}" ]`)
    return errno === 0
  }
}

/**
 * Open a website/link externally
 * @param {string} link - URL to open
 * @returns {Promise<void>}
 * @throws {Error} If not running on KSU WebUI
 */
export async function openWebsite(link) {
  if (!isKSUWebUI()) {
    throw new Error('Not running on KSU WebUI')
  }

  setTimeout(() => {
    if (isRunningOnWebUIX()) {
      const webui = new WebUI()
      const intent = new Intent(Intent.ACTION_VIEW)
      intent.setData(link)
      webui.startActivity(intent)
    } else {
      exec(`/system/bin/am start -a android.intent.action.VIEW -d ${link}`).then(({ errno }) => {
        if (errno !== 0) {
          const failed_toast = getTranslation("toast.failed_open_extrenal_browser")
          toast(failed_toast)
        }
      })
    }
  }, 100)
}

/**
 * List installed user applications (3rd party apps)
 * @returns {Promise<string[]>} Array of package names
 */
export async function listApps() {
  if (typeof ksu.listUserPackages !== 'undefined') {
    // KernelSU API - get user packages
    return JSON.parse(ksu.listUserPackages())
  } else if (isKSUWebUI()) {
    // WebUI X or other webui engine - use pm list packages -3 to get only 3rd party apps
    const { errno, stdout, stderr } = await exec('pm list packages -3')

    if (errno !== 0) {
      throw new Error(`Failed to list user packages: ${stderr}`)
    }

    // Parse output: "package:com.example.app" -> "com.example.app"
    return stdout
      .split('\n')
      .filter((line) => line.startsWith('package:'))
      .map((line) => line.substring(8).trim())
  } else {
    throw new Error('Not running on KSU WebUI')
  }
}

/**
 * Get application label (name) for a package with better fallback
 * @param {string} packageName - Package name to get label for
 * @returns {Promise<string>} Application label/name
 */
export async function getAppLabel(packageName) {
  try {
    if (typeof ksu.getPackagesInfo !== 'undefined') {
      // KernelSU API
      const packageNamesJson = JSON.stringify([packageName])
      const result = JSON.parse(ksu.getPackagesInfo(packageNamesJson))

      if (result.length > 0 && result[0].error) {
        throw new Error(result[0].error)
      }

      return result[0].appLabel || packageName
    } else if (typeof packageManagerInterface?.getApplicationInfo !== 'undefined') {
      // WebUI X API
      const info = packageManagerInterface.getApplicationInfo(packageName, 0, 0)
      return info.getLabel() || packageName
    } else {
      // No API available - return package name as fallback
      console.warn(`[getAppLabel] No API available, using package name for: ${packageName}`)
      return packageName
    }
  } catch (error) {
    console.error(`[getAppLabel] Failed for ${packageName}:`, error)
    return packageName // Fallback to package name
  }
}

/**
 * Batch get application labels for multiple packages with better fallback
 * @param {string[]} packageNames - Array of package names
 * @returns {Promise<Object[]>} Array of objects with packageName and appName
 */
export async function getBatchAppLabel(packageNames) {
  try {
    if (typeof ksu.getPackagesInfo !== 'undefined') {
      // KernelSU API - supports batch operations
      const packageNamesJson = JSON.stringify(packageNames)
      const result = JSON.parse(ksu.getPackagesInfo(packageNamesJson))

      return result.map((info) => ({
        packageName: info.packageName,
        appName: info.appLabel || info.packageName,
      }))
    } else if (typeof packageManagerInterface?.getApplicationInfo !== 'undefined') {
      // WebUI X API - process individually
      const results = []
      for (const packageName of packageNames) {
        try {
          const info = packageManagerInterface.getApplicationInfo(packageName, 0, 0)
          results.push({
            packageName,
            appName: info.getLabel() || packageName,
          })
        } catch (error) {
          console.warn(`[getBatchAppLabel] Failed for ${packageName}, using package name`)
          results.push({
            packageName,
            appName: packageName,
          })
        }
      }
      return results
    } else {
      // No API available - return package names as app names
      console.warn('[getBatchAppLabel] No API available, using package names as app names')
      return packageNames.map((packageName) => ({
        packageName,
        appName: packageName,
      }))
    }
  } catch (error) {
    console.error('[getBatchAppLabel] Failed:', error)
    // Return package names as fallback
    return packageNames.map((packageName) => ({
      packageName,
      appName: packageName,
    }))
  }
}

/**
 * Get application icon as base64 data URL with better fallback
 * @param {string} packageName - Package name to get icon for
 * @param {number} size - Icon size in pixels (default: 100)
 * @returns {Promise<string>} Base64 data URL of the icon
 */
export async function getAppIcon(packageName, size = 100) {
  try {
    if (typeof ksu !== 'undefined' && typeof ksu.listPackages !== 'undefined') {
      // Use ksu://icon/ URL scheme
      return `ksu://icon/${packageName}`
    } else if (typeof packageManagerInterface?.getApplicationIcon !== 'undefined') {
      // WebUI X API
      try {
        const stream = packageManagerInterface.getApplicationIcon(packageName, 0, 0)

        if (!stream) {
          throw new Error('Icon stream is null')
        }

        const wrappedStream = await wrapInputStream(stream)
        const arrayBuffer = await wrappedStream.arrayBuffer()

        if (!arrayBuffer || arrayBuffer.byteLength === 0) {
          throw new Error('Icon data is empty')
        }

        const base64 = arrayBufferToBase64(arrayBuffer)
        return 'data:image/png;base64,' + base64
      } catch (error) {
        console.error(`[getAppIcon] Failed to get icon for ${packageName}:`, error)
        throw error
      }
    } else {
      // No API available
      console.warn(`[getAppIcon] No API available for: ${packageName}`)
      throw new Error('No supported icon API available')
    }
  } catch (error) {
    console.warn(`[getAppIcon] Using fallback for ${packageName}`)
    return '' // Return empty string to trigger fallback icon
  }
}

/**
 * Batch get application icons for multiple packages with better fallback
 * @param {string[]} packageNames - Array of package names
 * @param {number} size - Icon size in pixels (default: 100)
 * @returns {Promise<Object[]>} Array of objects with packageName and icon
 */
export async function getBatchAppIcons(packageNames, size = 100) {
  try {
    if (typeof ksu !== 'undefined' && typeof ksu.listPackages !== 'undefined') {
      // Use ksu://icon/ URL scheme
      return packageNames.map((packageName) => ({
        packageName,
        icon: `ksu://icon/${packageName}`,
      }))
    } else if (typeof packageManagerInterface?.getApplicationIcon !== 'undefined') {
      // WebUI X API - process individually
      const results = []

      for (const packageName of packageNames) {
        try {
          const stream = packageManagerInterface.getApplicationIcon(packageName, 0, 0)

          if (!stream) {
            results.push({
              packageName,
              icon: '',
            })
            continue
          }

          const wrappedStream = await wrapInputStream(stream)
          const arrayBuffer = await wrappedStream.arrayBuffer()

          if (!arrayBuffer || arrayBuffer.byteLength === 0) {
            results.push({
              packageName,
              icon: '',
            })
            continue
          }

          const base64 = arrayBufferToBase64(arrayBuffer)
          const iconDataUrl = 'data:image/png;base64,' + base64
          results.push({
            packageName,
            icon: iconDataUrl,
          })
        } catch (error) {
          console.error(`[getBatchAppIcons] Failed for ${packageName}:`, error)
          results.push({
            packageName,
            icon: '',
          })
        }
      }
      return results
    } else {
      // No API available - return empty icons
      console.warn('[getBatchAppIcons] No API available, returning empty icons')
      return packageNames.map((packageName) => ({
        packageName,
        icon: '',
      }))
    }
  } catch (error) {
    console.error('[getBatchAppIcons] Failed:', error)
    // Return empty icons as fallback
    return packageNames.map((packageName) => ({
      packageName,
      icon: '',
    }))
  }
}

/**
 * Convert array buffer to base64 string
 * @param {ArrayBuffer} buffer
 * @returns {string}
 */
function arrayBufferToBase64(buffer) {
  const uint8Array = new Uint8Array(buffer)
  let binary = ''
  uint8Array.forEach((byte) => (binary += String.fromCharCode(byte)))
  return btoa(binary)
}
