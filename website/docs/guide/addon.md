---
title: "Encore Tweaks Addon"
description: "Explore the Encore Tweaks Addon for dynamic performance optimization, API integration, and seamless module collaboration."
---

# Encore Tweaks Addon

In addition to applying various performance tweaks dinamically, Encore Tweaks also provide API for other modules to interact and work together as profile changes.

Other module can watch current profile on Encore Tweaks and then apply their own tweaks as it changes, ultimately working together with Encore Tweaks as an Add-on.

::: tip
If you find that the existing API doesn't meet your needs or is inconvenient to use, you're welcome to give us suggestions [here](https://github.com/Rem01Gaming/encore/issues)!
:::

::: tip Note
All features are supported starting on Encore Tweaks 4.5 unless noted.
:::

## File Interface

### `/data/adb/.config/encore/current_profile`

**Description**: Contains the current performance profile state as a numeric value

**Possible Values**:

| Value | Profile Mode    | Description                                |
|-------|-----------------|--------------------------------------------|
| 0     | Perfcommon      | Common performance optimization on startup |
| 1     | Performance     | Performance mode                           |
| 2     | Normal          | Default operating mode                     |
| 3     | PowerSave       | Battery saving mode                        |

### `/data/adb/.config/encore/gameinfo`

**Description**: Contains active game session information when games from the Encore Tweaks gamelist are running

**Format**:
```
<package_name> <PID> <UID>
```

**Special Values**:
- `NULL 0 0` when no game is active

## Logging System

Encore Tweaks records events and errors in `/data/adb/.config/encore/encore.log` using a format similar to logcat. While consolidating logs is advantageous for debugging purposes, it poses challenges for other addons due to potential write conflicts and format discrepancies.

With the release of Encore Tweaks 4.6, Encore has provided a straightforward method for addons to log their information through `encore_log`.

```
Usage: encore_log <TAG> <LEVEL> <MESSAGE>
Levels: 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR, 4=FATAL

Example:
encore_log MyAddon 0 Hello World!

To see if the info successfully logged, run 'encore_utility logcat' or use Encore Tweaks WebUI.
```
