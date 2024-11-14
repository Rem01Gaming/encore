---
title: "WebUI Configuration Guide"
description: "Learn how to configure Encore Tweaks settings via Module WebUI for optimized performance on Android devices."
---

# Module WebUI and Configuration

The Encore Tweaks module provides a range of settings accessible through [Module WebUI](https://kernelsu.org/guide/module-webui.html), a KernelSU feature that lets modules build interactive HTML, CSS, and JavaScript interfaces. Through this WebUI, users can conveniently configure Encore Tweaks and adjust performance options.

While KernelSU and APatch integrate WebUI natively in their managers, Magisk does not support WebUI natively. To use WebUI in Magisk, you can use [this app](https://t.me/rem01schannel/636).

::: details Preview of Encore Tweaks WebUI
![Encore Tweaks WebUI](/Screenshot_20241011-095035_KernelSU.png)
:::

## Configuration Options

### Kill Logger
This option will stops the logger services and silence logs. Enable this option to reduces performance overhead.

::: warning
This option may breaks functionality on some apps that depends on `logd` service such as Shizuku.
:::

### Default CPU Governor
Select the default CPU governor for both "Normal" and "Powersave" profiles. The CPU governor determines power and performance trade-offs.

### Gameplay CPU Governor
Choose the CPU governor specifically for the "Performance" profile, which is optimized for high-demand tasks and gaming.

### Edit Gamelist
Edit the list of games and other performance-intensive applications. When these apps are active, Encore Tweaks will automatically apply the "Performance" profile.

### Save Logs
Enable this to save `encore-service` logs to internal storage at `/sdcard/encore_log`. This log can help with troubleshooting.

### Restart Service
Restarts the `encore-service` to apply changes immediately or refresh the service if it encounters issues.
