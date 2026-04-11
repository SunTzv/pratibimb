# DEV_LOG.MD: Project Pratibimb 
**Duration:** Fri Apr 10 14:31:26 2026 -> Sun Apr 12 00:15:42 2026
**Author:** Rishabh Pandey (SunTzv)
**Status:** Deployed & Survived.

This project was supposed to be a simple bridge between the Windows desktop and the browser's New Tab page. It escalated into a 34-hour war across four different architectural domains: Frontend UI, C++ Memory Management, Chromium Security Protocols, and OS-level Sandboxing. 

Here is the autopsy of what actually happened.

### Phase 1: The UI & Frontend
Getting the data was only half the battle; making it look good and load fast was a nightmare.
* **The White Flash Problem:** Initially, `newtab.html` would blind the user with a white flash before the C++ host could send the image. Implemented aggressive Base64 image caching in JavaScript/local storage so the UI loads instantly (`commit d4d22989`), falling back to the cache while the background script fetches the fresh frame.
* **Thematic Consistency:** Spent hours tweaking the exact palette (`commits f3b7b5e5 -> af3ca6f3`). Got the cyber-retro, dark-mode aesthetic dialed in perfectly. The UI had to feel native, not like a bolted-on webpage. 
* **Manifest Permissions:** Fought with `manifest.json` storage and activeTab permissions to ensure the extension could actually hold the cached data without triggering Chromium's bloatware alarms.

### Phase 2: The C++ Engine & Memory 
Writing a background daemon that doesn't leak memory or lock up the CPU.
* **Image Compression:** Raw bitmap data from the Windows TranscodedWallpaper file is massive. Integrated `stb_image` and `stb_image_write` (`commit 8692a247`) to instantly compress the raw buffer into an 80-quality JPG in memory before encoding it.
* **Memory Leaks:** Hit memory issues early on (`commit 1e1d5b34`). Had to ensure `stbi_image_free` and vector buffers were being immediately destroyed after the Base64 encoding finished to prevent the host from ballooning in RAM.
* **The Chunking Algorithm:** Chromium pipes choke on massive single payloads. Had to split the Base64 string into 500KB-800KB chunks and stream them sequentially to the JavaScript listener.

### Phase 3: The Deployment Hell
An app is useless if it only works on the dev machine. 
* **The Setup Automator:** Wrote a custom C++ installer/uninstaller (`commit c80dfeda`) to automatically handle the Native Messaging JSON creation, path resolution, and Registry Key injection (`HKCU\Software\...\NativeMessagingHosts`).
* **DLL Bruh (`commit f2375813`):** The `.exe` kept failing on other environments because MinGW dynamically links standard libraries. Forced `-static -static-libgcc -static-libstdc++` so the executable carries its own dependencies.

### Phase 4: The Protocol & OS Boss Fights
The final 12 hours (`commits 449a4a0a -> 7f951879`). Mathematically perfect code being blocked by invisible walls.
* **The Binary Trap:** Windows 11 tries to format `cout` with `\r\n`, which shatters the 4-byte Chromium length header. Enforced `_setmode(_O_BINARY)` to lock the pipeline.
* **The Heartbeat:** The host kept exiting too early. Wrapped the C++ `cin.read` in a `while` loop to keep the process alive as long as the browser held the port open.
* **Smart App Control (SAC):** Windows 11 silently assassinated the unsigned `.exe`. Had to disable SAC to let the native host breathe.
* **The Brave Sandbox Anomaly:** The final "very tuff bug." The extension worked flawlessly on Edge (Win11) and Brave (Tiny10), but Brave on Win11 flat-out refused to launch the host. Discovered that aggressive UWP sandboxing and ASR rules block non-Microsoft browsers from launching unsandboxed local binaries on locked-down Windows builds. 

### Core Takeaway
Systems programming is 20% writing logic and 80% fighting the environment. The UI caching makes it feel seamless, the C++ chunking makes it fast, and the static compiling makes it portable. I beat the Native Messaging protocol.