---
title: "WebUI Configuration Guide"
description: "Learn how to configure Encore Tweaks settings via Module WebUI for optimized performance on Android devices."
---

# Module WebUI and Configuration

The **Encore Tweaks** module offers a variety of performance-tuning settings accessible through the [Module WebUI](https://kernelsu.org/guide/module-webui.html)—a feature of **KernelSU** that allows modules to build interactive HTML, CSS, and JavaScript interfaces. This WebUI makes it easy for users to configure Encore Tweaks without needing to manually edit configuration files.

While **KernelSU** and **APatch** support WebUI natively in their managers, **Magisk** does not. If you're using Magisk, you can still access the WebUI using the [KSU WebUI APK](https://t.me/rem01schannel/636) or [MMRL](https://github.com/DerGoogler/MMRL).

::: details Preview of Encore Tweaks WebUI
![Encore Tweaks WebUI](/Screenshot_20250520-161223_MMRL.png)
:::

## Configuration Options

### Kill Logger
Stops system logging services to reduce background activity and performance overhead.

::: tip Deprecated
This feature has been removed from the 4.3 release.
:::

::: warning
Do **NOT** enable this if you're debugging ROMs or apps. It disables system logging entirely, which may cause issues with apps that rely on `logd`.
:::

### DND on Gameplay
Automatically enables **Do Not Disturb** mode while gaming to block notifications and interruptions, ensuring an uninterrupted experience.

### Lite Mode
This setting will minimize overheating and reduce power usage while gaming by allowing the CPU and other parts to operate at lower frequencies instead of always running at peak performance; however, this choice will impact the overall performance of the game.

### Default CPU Governor
Sets the CPU governor used in the **Normal** profile. The governor determines how the CPU scales frequency based on system load, affecting both power efficiency and performance.

### Powersave CPU Governor
Defines the CPU governor for the **Powersave** profile, optimized for battery life. If not explicitly set, it will use the default CPU governor.

### Edit Gamelist
Allows you to manage a list of games and performance-demanding apps. When any listed app is launched, Encore Tweaks automatically switches to the **Performance** profile.

### Save Logs
Stores Encore daemon logs in internal storage for troubleshooting purposes. These logs can help diagnose issues or confirm profile behavior.