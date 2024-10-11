---
title: "Module WebUI and Configuration"
description: "How to configure Encore Tweaks through Module Module WebUI"
---

# Module WebUI and Configuration
Encore Tweaks exposes some settings and utilities inside [Module WebUI](https://kernelsu.org/guide/module-webui.html), a KernelSU feature that allows modules to write HTML + CSS + JavaScript pages through any web technology and displaying UI interfaces and interacting with users.

::: details Encore's WebUI Preview
![Encore Tweaks Module WebUI](/Screenshot_20241011-095035_KernelSU.png)
:::

## Kill Logd
As name suggests, this will kill logd service which is logging system in Android. enabling this will help reduce performance overhead.

::: warning
Do NOT enable this if you're going to test ROMs and Apps, this will literally silence your Android logging system !
:::

## Default CPU Governor
CPU Governor that will used in Normal and Powersave profile.

## Performance profile CPU Gov
CPU Governor that will used in Performance profile.

## Edit Gamelist
Edit game and other performance intensive apps that will trigger Performance profile.

## Save Logs
Save `encore-service` logs into internal storage, more precisely <code>/sdcard/encore_logs</code>.

## Restart Service
Restart fresh `encore-service`.
