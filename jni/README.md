# Encore Tweaks Daemon

Before you digging into this code thinking this is some kind of scheduling module like Uperf, it's not. Encore Tweaks is a profile-style performance module, it simply applies performance tweaks as profiles and  <ins>do not dinamically control the scheduling and frequencies</ins>.

Encore Tweaks works by using information such as:
- Currently running app (window focussed)
- Screen state, whenever it's awake or not and...
- Battery saver state (yes the ones on your qs)

All of those information fetched from dumpsys which you can see the logic [here](src/encore_profiler.c).

## Project Structure

- include/* -> headers
- src/* -> Other functions (files named semantically)
- main.c -> Main daemon code
- Android.mk -> Android NDK thing
- Application.mk -> Android NDK thing

## Workflow diagram

![Workflow diagram of Encore Tweaks daemon](./diagram.webp)
