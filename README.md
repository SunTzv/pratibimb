# Pratibimb

A Chromium browser new tab extension that syncs the new tab background with the current Windows desktop wallpaper.

See `DOCUMENTATION.md` for full project documentation and architecture details.

## Installation

1. Build `host/main.cpp` into `host/pratibimb_host.exe`.
2. Place a prebuilt `install\pratibimb_installer.exe` inside the `install\` folder, or compile `install\installer.cpp` locally.
3. Run `install\pratibimb_installer.exe`.
4. Use the GUI to install or uninstall the native backend and optionally register the extension using a local folder or `.crx` file path.

> If you want to ship the extension without developer mode, use the `.crx` package. Keep `extension.pem` with the CRX to preserve the extension ID.

> git commit -m "sync your wallpapers in style?"