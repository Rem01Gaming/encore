---
title: "Encore Tweaks Addon"
description: "Explore the Encore Tweaks Addon for dynamic performance optimization, API integration, and seamless module collaboration."
---

# Encore Tweaks Addon

In addition to applying various performance tweaks dinamically, Encore Tweaks also provide API for other modules to interact and work together as profile changes.

Other module can watch current profile on Encore Tweaks and then apply their own tweaks as it changes, ultimately working together with Encore Tweaks as an Add-on.

## File Interface

### `/dev/encore_mode`

**Description**: Contains the current performance profile state as a numeric value

**Possible Values**:

| Value | Profile Mode    | Description                                |
|-------|-----------------|--------------------------------------------|
| 0     | Perfcommon      | Common performance optimization on startup |
| 1     | Performance     | Maximum performance mode                   |
| 2     | Normal          | Default operating mode                     |
| 3     | PowerSave       | Battery saving mode                        |

### `/dev/encore_game_info`

**Description**: Contains active game session information when games from the Encore Tweaks gamelist are running

**Format**:
```
<package_name> <PID> <UID>
```

**Special Values**:
- `NULL 0 0` when no game is active

---

If you find that the existing API doesn't meet your needs or is inconvenient to use, you're welcome to give us suggestions [here](https://github.com/Rem01Gaming/encore/issues)!

## Some tips

1. Even though we could use same log file as Encore Tweaks (`/dev/encore_log`) for convenience, Addon modules should handles how the log is being written on the file to prevent write conflicts.
2. We recommend using `inotify` to monitor file changes, there's also **[x-watcher](https://github.com/nikp123/x-watcher)** Library that conveniently handles `inotify` for you.
