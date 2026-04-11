# 🔁 Pratibimb

---

### ⚡ **Jump to:** [**Features**](#-features) • [**Installation**](#-installation) • [**Uninstallation**](#-uninstallation) • [**Architecture**](#-architecture)

---

> *Sync your wallpapers in style.*

**Pratibimb** (Hindi for *reflection*) is a premium, lightweight Chromium extension that seamlessly synchronizes your browser's New Tab background with your active Windows desktop wallpaper. 

Powered by a lightning-fast C++ Native Messaging host and a zero-config native installer, Pratibimb bridges the gap between your local operating system and your browser with zero latency.

---

## ✨ Features

* **Universal Chromium Support:** Works flawlessly on Google Chrome, Microsoft Edge, Brave, Vivaldi, Chromium, and Opera.
* **Native C++ Backend:** Uses a hyper-optimized Windows API Native Messaging host (`pratibimb_host.exe`) to fetch wallpaper data instantly without background bloat.
* **Monolithic "Zero-Config" Installer:** A sleek, Dark Mode Windows 11 GUI installer that automatically unpacks payloads, sets up secure AppData directories, and writes registry policies in a single click.
* **Modern Aesthetic:** The New Tab page features dynamic lighting, glassmorphism UI, QOL weather caching, and debounced search integrations.

---

## 🚀 Installation

[![Download Pratibimb](https://img.shields.io/badge/Download-Pratibimb__Setup.exe-0078D4?style=for-the-badge&logo=windows)](install/Pratibimb_Setup.exe)

Because Chromium browsers have strict security rules against sideloading local extensions, installation is a quick two-step process to ensure everything links securely.

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