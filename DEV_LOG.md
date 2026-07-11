# DEV_LOG.MD — Project Pratibimb
**Status:** Somehow works. 
**Author:** Rishabh Pandey (SunTzv)

What started as "lemme sync my wallpaper to the browser new tab" accidentally became a side quest into browser protocols, Windows weirdness, memory management, and emotional damage.

## Release Log

### v1.0.0 — First Release *(Apr 12, 2026)*

- Fixed native messaging host reading wallpaper path from registry
- Fixed silent crash on wallpaper paths with spaces

---

### v1.3.2 *(May 8–19, 2026)*

Came back and worked on it again because apparently I enjoy suffering.

- Added idle mode — extension stops polling when system is idle
- Proper icons instead of emojis (we are professionals now)
- Visual polish pass
- Fixed double `.png` extension being appended to wallpaper paths
- Made the installer sundar

---

### v1.4.3 *(May 25-26, 2026)*

Didn't plan to open this repo today. Opened it anyway.

- New UI pass — something felt off, now it doesn't
- Missed a file (there's always one)
- Added customization options

---

### v1.4.3+ *(June 13, 2026)*

Decided it's time to show some love to the penguin.

- **Linux Support Added**
- **Linux CLI Installer**
- Improved installtion with a hardcoded key haven't added this to the windows version yet

---

### v1.6.4 *(July 10-11, 2026)*

Because maintaining zip files is a waste of human potential.

- Unfortunately I don't offer Pratibimb_Setup.exe anymore, that's cuz I migrated to CLI install lmaoo
- The installer now pulls the latest release directly from GitHub so I never have to manually make zip files again.
- Added quick update commands so you can update just the extension without rebuilding the host.
- The search bar can act as an AI prompt directly by hitting `Ctrl + Enter` while searching.
- Cleaned up the AI script to just securely paste your prompt without wasting your usage limits.
- Added icons to the manifest finally looks good now 😭.

---

## Takeaway

Systems programming is basically:
- 20% writing code
- 80% fighting your OS, browser, compiler, and existence

But hey, it works now. Mostly.