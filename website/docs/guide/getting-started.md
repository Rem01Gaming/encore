---
title: Getting Started
description: Learn how to install and configure Encore Tweaks settings via Module WebUI for optimized performance on Android devices.
---

# Getting Started

Getting started with Encore Tweaks is easy as it designed to be user-friendly.

## Installation

Install Encore Tweaks module just like any other modules:

1. [Download Encore Tweaks](https://encore.rem01gaming.dev/download) from the official website.
2. Flash it via Magisk/KernelSU/APatch.
3. Reboot your device, and you're ready to go!

## Profiles

Encore Tweaks dynamically adjusts your device's performance based on real-time data, such as the currently running app, screen state (awake or not), and battery saver mode. Using this information, it seamlessly switches between different profiles to optimize performance or save power as needed.

In contrast, from scheduler modules such as Uperf and Tritium, Encore Tweaks works in more traditional way compared to them, this gives more stability and compatibility to older devices that doesn't support eBPF.

### Performance Profile

Performance profile will boost your device performance to improve responsiveness and stability during intensive tasks, automatically applied when launching your favorite game.

---

### Balanced Profile

Default system behavior without any special tweaks, automatically applied when not running any game.

---

### Powersave Profile

Maximizes power efficiency for extended battery life, limits non-essential component frequencies. Automatically applied when battery saver option in quick settings are enabled.

## Configuration

The Encore Tweaks settings is accessible through the Module WebUI, a feature of KernelSU that allows modules to build interactive HTML, CSS, and JavaScript interfaces. This WebUI makes it easy for users to configure Encore Tweaks without needing to manually edit configuration files.

While KernelSU and APatch support WebUI natively in their managers, Magisk does not. If you're using Magisk, you can still access the WebUI using the [WebUI X](https://github.com/MMRLApp/WebUI-X-Portable).

:::tip Note
You may need to [update your WebView](https://play.google.com/store/apps/details?id=com.google.android.webview) to use the WebUI, old WebView versions are not supported by Encore Tweaks WebUI.
:::
