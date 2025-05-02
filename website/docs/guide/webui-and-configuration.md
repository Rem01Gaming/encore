---
title: "WebUI Configuration Guide"
description: "Learn how to configure Encore Tweaks settings via Module WebUI for optimized performance on Android devices."
---

# Module WebUI and Configuration

The Encore Tweaks module provides a range of settings accessible through [Module WebUI](https://kernelsu.org/guide/module-webui.html), a KernelSU feature that lets modules build interactive HTML, CSS, and JavaScript interfaces. Through this WebUI, users can conveniently configure Encore Tweaks and adjust performance options.

While KernelSU and APatch integrate WebUI natively in their managers, Magisk does not support WebUI natively. To use WebUI in Magisk, you can use [KSU WebUI APK](https://t.me/rem01schannel/636) or [MMRL](https://github.com/DerGoogler/MMRL).

::: details Preview of Encore Tweaks WebUI
![Encore Tweaks WebUI](/Screenshot_20250206-175121_MMRL.png)
:::

## Configuration Options

### Kill Logger
This option will stops the logger services and silence logs. Enable this option to reduces performance overhead.

::: warning
Do NOT enable this option if you're testing ROMs or apps as it will completely disable system logging. Software that depends on `logd` service will not working properly!
:::

### DND on Gameplay
Enable do not disturb mode when playing games. limits interruptions from notifications and calls, allowing you to play your favorite games without any distraction.

### Default CPU Governor
Default CPU governor to be used on Normal profile. The CPU governor determines power and performance trade-offs.

### Powersave CPU Governor
Choose the CPU governor specifically for the Powersave profile, which is optimized for battery life, by default this option will follow default CPU governor.

### Edit Gamelist
Edit the list of games and other performance-intensive applications. When these apps are active, Encore Tweaks will automatically apply the "Performance" profile.

### Save Logs
Save Encore daemon logs to internal storage, this log can help with troubleshooting.
