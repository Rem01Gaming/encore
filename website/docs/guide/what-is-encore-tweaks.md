---
title: "What is Encore Tweak"
description: "Encore Tweaks is a free and open source performance Magisk module designed to boost device performance while playing games but keeping battery life on normal usage"
---

# What is Encore Tweaks?
Encore Tweaks is a free and open source performance Magisk module designed to boost device performance while playing games but keeping battery life on normal usage.

## Features
The main feature of Encore Tweaks is it is **Fully Automatic**. Rather than manually selecting which performance profile you want using cli, app or other gui, Encore Tweaks automatically applies performance profile based on what app that user open without any user settings.

And also, Encore Tweaks provides wide support for vast majority of popular SoCs like Snapdragon, Mediatek, Exynos, Google Tensor, and even Unisoc with tailored tweaks for each one.

## About 'Automatic' stuff
Encore Tweaks is fully automatic while comes to performance profile, well you may know this feature as 'AI feature' on other modules. Service will choose the profile according to this 3 rules:

### Performance Profile
Service will continuously checking any apps or games that listed on Gamelist and If user open any of it, service will apply performance mode. this included various kernel parameters, CPU, CPU Bus, GPU, and DRAM frequencies will be locked to highest possible OPP. This profile will be maintained until the user close the game.

this profile will also set game and performance-intensive app to highest possible priority for CPU and I/O resources among other running processes to ensure performance, minimize jitter and latency.

### Powersave Profile
If user enables battery saver mode (except while charging), service will apply powersave mode. powersave mode will lock CPU Bus, GPU, and DRAM frequencies to lowest possible OPP for saving power.

::: info
Powersave profile cannot override Performance profile.
:::

### Normal Profile
If any of criteria on above not satisfied, service will apply normal mode. this mode will remove any restrictions and tweaks on other profiles.

## How to install Encore Tweaks
- Download the module from [here](/download)
- Flash in Magisk/KernelSU/APatch and reboot
- And that's it ;)

## How to configure Encore Tweaks
Please refer to [Module WebUI and Configuration](/guide/webui-and-configuration)

## I have a question 🤔

Check out our [FAQ here](/guide/faq), if you don't find your question there feel free to ask on our Telegram Group [@rem01shideout](https://t.me/rem01shideout).
