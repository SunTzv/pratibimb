# Pratibimb

> *Your browser finally has the same energy as your desktop.*

***Pratibimb*** (Hindi for *reflection*) is a Chromium extension that syncs your Windows or Linux desktop wallpaper to your New Tab page — live, automatically, every time. The whole UI adapts to whatever wallpaper you're rocking. Moody dark wallpaper? Moody vibes. Bright summer meadow? You get the idea.

---

<div align="center">
  <img src="assets/hydrated_sap.png" width="49%" />
  <img src="assets/night_light.png" width="49%" />
  <br>
  <sub>active</sub>
  <br><br>
  <img src="assets/old_town_of_bern.png" width="49%" />
  <img src="assets/sakura_canopy.png" width="49%" />
  <br>
  <sub>idle</sub>
  <br><br>
  <sub>looks different every time. that's the whole point.</sub>
</div>

---

## How it works

A tiny C++ program reads your wallpaper straight from Windows and passes it to the extension instantly — no background tasks, no polling, no funny business. The New Tab page then builds itself around it: lighting, glassmorphism, weather, search bar, all of it.

You also get a one-click installer that handles all the annoying registry and AppData stuff so you don't have to.

---

## Before you install — read this (it's short, I promise)

Windows 11 is *very* dramatic about local `.exe` files talking to browsers. Here's what might trip you up:

- **Defender might quietly block it** with zero explanation. If it's not working, add `%LOCALAPPDATA%\Pratibimb` as a Defender exclusion.
- **Store-installed browsers can't do this at all.** If you got your browser from the Microsoft Store, grab the direct installer from the browser's website instead.

**What actually works:**

| Setup | Verdict |
|---|---|
| Linux (just tested Cinnamon so far) | ✅ CLI setup |
| Tiny10 / Win10 — Chrome, Brave, Edge | ✅ Perfect |
| Windows 11 — Edge | ✅ Perfect |
| Windows 11 — Brave | I FOUND A FIX I'LL UPDATE SOON PROMISE |
| Windows 11 — Chrome | ✅ Perfect |

---

## Installation

### Windows
[![Download](https://img.shields.io/badge/Download-Pratibimb__Setup.exe-0078D4?style=for-the-badge&logo=windows)](https://github.com/SunTzv/Pratibimb/releases/latest/download/Pratibimb_Setup.exe)

**Step 1** — Run `Pratibimb_Setup.exe`, pick your browser, click **Extract & Open Folder**.

**Step 2** — Go to `chrome://extensions` (or `edge://extensions`), turn on **Developer mode**, and drag the `extension` folder in. Copy the Extension ID it gives you, paste it back into the installer, and click **Link Native Host**.

Open a new tab. That's it. 🎉

### Linux
[![Download](https://img.shields.io/badge/Download-Pratibimb__Linux.tar.gz-FCC624?style=for-the-badge&logo=linux&logoColor=black)](https://github.com/SunTzv/Pratibimb/releases/latest/download/Pratibimb-Linux.tar.gz)
[![Download](https://img.shields.io/badge/Download-extension.zip-8A2BE2?style=for-the-badge)](https://github.com/SunTzv/Pratibimb/releases/latest/download/extension.zip)

**Step 1** — Extract the `.tar.gz` archive and navigate into it via your terminal.

**Step 2** — Go to your browser's extensions page, turn on **Developer mode**, and drag the `extension` folder or extracted `extension.zip` into it.

**Step 3** — Run `./linux_install/install.sh` and hit `Y`. It auto-compiles the C++ host and statically links it to Chrome, Chromium, Brave, and Edge automatically!

Open a new tab. That's it. 🎉

*(Already ran the script but need an update? Just replace your old `extension` folder with the new one. The host config is fully persistent!)*

---

## Uninstalling

**Windows:** Run the installer → Uninstall → **UNINSTALL EVERYTHING**. Removes the host, the registry keys, the AppData folder — all of it. Then remove the extension from your browser. Gone without a trace.

**Linux:** Run `./linux_install/uninstall.sh`. Wipes the generated JSON configs from your browsers and purges the binary from `~/.local/share/Pratibimb`. Remove the extension from your browser. Gone without a trace.

---

## Go deeper

[`DEV-LOG.md`](DEV_LOG.md) — the build journal, mistakes and all
(nothing for you mostly for me to track versions)

---

<sub>Favicon by Dupe (dupe_dupe on Discord) · Weather icons by <a href="https://basmilius.github.io/meteocons/fill.html">Bas Milius</a> · Fonts via <a href="https://github.com/TeaTimBuYT">TeaTimBuYT</a></sub>