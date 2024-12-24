---
title: "What is Encore Tweaks?"
description: "Encore Tweaks is an performance Magisk module designed to boost device performance for gaming while preserving battery life during regular use."
---

# What is Encore Tweaks?

Encore Tweaks is a Magisk Module created to enhance device performance during gaming sessions, while keeping battery life optimized for normal use. With **fully automatic performance profiles** and **wide compatibility** across various SoCs, Encore Tweaks adapts dynamically to boost your device's performance when it matters most.

## Key Features

- **Automatic Performance Profiles**: No need to manually adjust performance settings; Encore Tweaks automatically switches profiles based on app usage, allowing you to focus on what matters.
- **Optimized for Popular SoCs**: Supports a wide range of System-on-Chips (SoCs), including Snapdragon, MediaTek, Exynos, Google Tensor, and even Unisoc, with custom optimizations tailored for each.
- **Lag-Free Gaming Experience**: By prioritizing resources for gaming apps, Encore Tweaks minimizes lag, jitter, and latency for smoother gameplay.

## About the Automatic Profiles

Encore Tweaks includes **three performance profiles** - Performance, Powersave, and Normal. The module seamlessly switches profiles based on app usage, battery settings, and other criteria.

### Performance Profile
When user open an app or game listed in the **Gamelist**, Encore Tweaks automatically applies **Performance Mode**, setting CPU, CPU Bus, GPU, and DRAM frequencies to their highest operating points (OPP). This profile also prioritizes CPU and I/O resources for intensive apps, reducing lag and jitter for a stable, responsive experience.

### Powersave Profile
**Powersave Mode** activates automatically when the device is in battery saver mode (excluding while charging). This mode limits CPU, GPU, and DRAM frequencies to their lowest OPP to conserve power. Note that the Powersave Profile will not override the Performance Profile when it's needed.

::: info
Powersave Mode is prioritized only when no performance-intensive apps are active.
:::

### Normal Profile
The **Normal Mode** applies when neither Performance nor Powersave conditions are met. This profile removes all special tweaks, allowing the device to operate normally.

## Installation

1. [Download Encore Tweaks](/download) from the official page.
2. Flash it via **Magisk/KernelSU/APatch**.
3. Reboot your device, and you're ready to go!

## Configuration

Encore Tweaks provides a user-friendly WebUI for managing settings and adjusting preferences. For details, visit the [Module WebUI and Configuration Guide](/guide/webui-and-configuration).

## FAQ

Have a question? Check out our [FAQ page](/guide/faq). If you still have questions, join our Telegram Group [@rem01shideout](https://t.me/rem01shideout).
