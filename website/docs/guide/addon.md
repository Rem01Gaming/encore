---
title: "Encore Tweaks Addon"
description: "Explore the Encore Tweaks Addon for seamless module collaboration."
---

# Encore Tweaks Addon

In addition to applying various performance tweaks dinamically, Encore Tweaks also provide API for other modules to interact and work together as profile changes.

Other module can watch current profile using Inotify on Encore Tweaks and then apply their own tweaks as it changes, ultimately working together with Encore Tweaks as an Add-on.

::: tip
If you find that the existing API doesn't meet your needs or is inconvenient to use, you're welcome to give us suggestions [here](https://github.com/Rem01Gaming/encore/issues)!
:::

::: tip Note
All features are supported starting on Encore Tweaks 4.5 unless noted.
:::

## File Interface

### `/data/adb/.config/encore/current_profile`

Contains the current profile state as a numeric value

**Possible Values**:

| Value | Profile Mode    | Description                                |
|-------|-----------------|--------------------------------------------|
| 0     | Perfcommon      | Common performance optimization on startup |
| 1     | Performance     | Performance mode                           |
| 2     | Normal          | Default operating mode                     |
| 3     | Powersave       | Battery saving mode                        |

### `/data/adb/.config/encore/gameinfo`

Contains active game session information when games from the Encore Tweaks gamelist are running

**Format**:
```
<package_name> <PID> <UID>
```

**Special Values**:
- `NULL 0 0` when no game is active
