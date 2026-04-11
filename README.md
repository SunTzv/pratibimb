# 🔁 Pratibimb

---

### ⚡ **Jump to:** [**Features**](#-features) • [**Compatibility & Warnings**](#️-compatibility--developer-warnings) • [**Installation**](#-installation) • [**Uninstallation**](#-uninstallation) • [**Architecture**](#-architecture)

---

> *Sync your wallpapers in style.*

**Pratibimb** (Hindi for *reflection*) is a premium, lightweight Chromium extension that seamlessly synchronizes your browser's New Tab background with your active Windows desktop wallpaper. 

Powered by a lightning-fast C++ Native Messaging host and a zero-config native installer, Pratibimb bridges the gap between your local operating system and your browser with zero latency.

---

## ✨ Features

* **Native C++ Backend:** Uses a hyper-optimized Windows API Native Messaging host (`pratibimb_host.exe`) to fetch wallpaper data instantly without background bloat.
* **Monolithic "Zero-Config" Installer:** A sleek, Dark Mode Windows 11 GUI installer that automatically unpacks payloads, sets up secure AppData directories, and writes registry policies in a single click.
* **Modern Aesthetic:** The New Tab page features dynamic lighting, glassmorphism UI, QOL weather caching, and debounced search integrations.

---

## ⚠️ Compatibility & Developer Warnings
**Please read this before installing.** Building a local Native Messaging host requires bypassing heavily guarded OS and browser security protocols. Installation can be tough depending on your setup. 

**The "Invisible Wall" on Windows 11:**
Modern Windows 11 treats unsigned local `.exe` files spawned by web browsers as highly suspicious. 
* **Smart App Control (SAC):** If you are on Windows 11, SAC or Windows Defender may silently block or quarantine `pratibimb_host.exe`. You may need to add an exclusion in Windows Defender for the `%LOCALAPPDATA%\Pratibimb` folder.
* **UWP Sandboxing:** Some browsers installed via the Microsoft Store are sandboxed and physically cannot launch local desktop binaries. 

**Tested Environments:**
* ✅ **Tiny10 (Windows 10):** Tested on Chrome, Brave, and Edge. Works flawlessly. (Tiny10 lacks the aggressive sandboxing of Win11).
* ✅ **Windows 11:** Tested on Edge and works flawlessly. 
* ❌ **Brave on Windows 11:** Brave's aggressive custom security and Windows 11 UWP sandboxing actively block local native hosts from modifying the New Tab page. 
* ❓ **Chrome / Opera / Vivaldi on Windows 11:** Unverified. I have not explicitly tested these on Win11, but they should theoretically work if Defender exclusions are set.

---

## 🚀 Installation

[![Download Pratibimb](https://img.shields.io/badge/Download-Pratibimb__Setup.exe-0078D4?style=for-the-badge&logo=windows)](install/Pratibimb_Setup.exe)

Because Chromium browsers have strict security rules against sideloading local extensions, installation is a quick two-step process to ensure everything links securely via the Windows Registry.

### Step 1: Run the Installer
1. Download and run **`Pratibimb_Setup.exe`**.
2. Select **Install** and check the box next to your active browser(s).
3. Click **`1. Extract & Open Folder`**.
   * *This will securely unpack the extension to your local AppData and automatically open the folder in Windows Explorer.*

### Step 2: Load & Link
1. Open your browser and navigate to the extensions page (e.g., `edge://extensions` or `chrome://extensions`).
2. Turn on **Developer mode**.
3. Drag and drop the newly opened **`extension`** folder directly into the browser window.
4. Copy the newly generated **Extension ID** (a 32-character string).
5. Go back to the Pratibimb installer, paste the ID into the box, and click **`3. Link Native Host`**.

🎉 **Done!** Open a new tab to see your synchronized desktop wallpaper.

---

## 🗑️ Uninstallation

Pratibimb leaves zero trace when uninstalled. 
1. Run `Pratibimb_Setup.exe`.
2. Select **Uninstall**.
3. Click **UNINSTALL EVERYTHING**. This wipes the Native Host, deletes the registry keys, and securely removes the hidden AppData folder.
4. Remove the extension from your browser's extension page.

---

## 📚 Architecture
For a deep dive into the Native Messaging protocol, the DWM dark mode UI, and the internal file architecture, please see [`DOCUMENTATION.md`](DOCUMENTATION.md).

---

## 💿💻 Dev-Log
For an insight on building progress check [`DEV-LOG.md`](DEV_LOG.md).

---

## Credits ©

> Favicon by Dupe (dupe_dupe, discord)
