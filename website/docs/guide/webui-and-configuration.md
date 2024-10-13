---
title: "How to configure"
description: "How to configure Encore Tweaks through Module WebUI"
---

# Module WebUI and Configuration
Encore Tweaks exposes some settings and utilities inside [Module WebUI](https://kernelsu.org/guide/module-webui.html), a KernelSU feature that allows modules to write HTML + CSS + JavaScript pages through any web technology and displaying UI interfaces and interacting with users.

While KernelSU and APatch can use WebUI natively on their manager, Magisk doesn't implement webroot on their manager and you need to use [this app](https://t.me/rem01schannel/636) to order to use WebUI in Magisk.

::: details Encore's WebUI Preview
![Encore Tweaks Module WebUI](/Screenshot_20241011-095035_KernelSU.png)
:::

## Kill Logd
As name suggests, this will stop logd service which is logging system in Android. enabling this will help reduce performance overhead.

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
Save `encore-service` logs into internal storage, more precisely `/sdcard/encore_log`.

## Restart Service
Restart fresh `encore-service`.
