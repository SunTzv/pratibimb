# Pratibimb Documentation

Pratibimb means "reflection", and that is exactly what this project tries to do:
make your browser's new tab feel like it belongs to your desktop.

Instead of showing a plain browser page, Pratibimb reads your current Windows
wallpaper, places it behind the new tab, and lets the rest of the page quietly
adapt around it. The clock, search bar, weather, colors, shadows, and glassy
surfaces all respond to the wallpaper so the tab feels alive instead of pasted
on.

This document explains the whole project in simple language. It is written for
future-you, contributors, and anyone who wants to know what is happening without
needing to decode every line first.

---

## The Short Version

Pratibimb has three main parts:

| Part | Where it lives | What it does |
|---|---|---|
| Browser extension | `extension/` | Replaces the New Tab page and draws the UI. |
| Native host | `host/` | A small Windows program that reads the desktop wallpaper. |
| Installer | `install/` | Extracts files and links the browser extension to the native host. |

The browser extension cannot read your Windows wallpaper by itself. Browsers do
not allow web pages to freely touch local system files, which is a good thing
for safety. So Pratibimb uses a native host: a tiny local Windows program that
the browser is allowed to talk to after setup.

The flow is:

1. You open a new tab.
2. The extension starts immediately.
3. It asks the native host for the current Windows wallpaper.
4. The native host reads the wallpaper from Windows.
5. The native host compresses it and sends it back in small pieces.
6. The extension joins the pieces, shows the wallpaper, and updates the page
   colors to match it.

That is the core trick.

---

## Project Map

```text
pratibimb/
  README.md
  DEV_LOG.md
  DOCUMENTATION.md
  extension.zip

  assets/
    belude_in_selgrade.png
    palace_of_winds.png
    roseate_glaze.png
    sunburn_wen.png

  extension/
    manifest.json
    newtab.html
    fastload.js
    newtab.js
    ui.js
    icons/

  host/
    main.cpp
    pratibimb_host.exe
    stb_image.h
    stb_image_write.h
    com.suntzv.pratibimb.json

  install/
    installer.cpp
    resources.rc
    resources.res
    Pratibimb_Setup.exe
```

The important thing to remember is this:

`extension/` is what the user sees.  
`host/` is what talks to Windows.  
`install/` is what connects the two.

---

## What The Extension Does

The extension is a Manifest V3 Chromium extension. Its manifest says:

```json
"chrome_url_overrides": {
  "newtab": "newtab.html"
}
```

That means whenever the browser opens a new tab, Chromium loads
`extension/newtab.html` instead of the normal new tab page.

The extension asks for these permissions:

| Permission | Why it is needed |
|---|---|
| `nativeMessaging` | Lets the extension talk to the local Windows host program. |
| `storage` | Lets the extension store small browser-side settings and cache data. |
| `unlimitedStorage` | Helps keep the wallpaper cache without tiny storage limits getting in the way. |

It also allows calls to:

| Website | Used for |
|---|---|
| `api.open-meteo.com` | Current weather. |
| `nominatim.openstreetmap.org` | Turning latitude/longitude into a city name. |
| `suggestqueries.google.com` | Search suggestions while typing. |

---

## The New Tab Page

The visible page is mostly in `extension/newtab.html`.

It contains:

- A large clock.
- A date line.
- Weather in the top-right corner.
- A centered search bar.
- Search suggestions.
- Small shortcut buttons for settings, extensions, history, and downloads.
- A soft greeting at the bottom.
- An idle mode that hides most of the UI after a few quiet seconds.

The design uses two main fonts from Google Fonts:

- `Cormorant Garamond` for the big elegant clock.
- `DM Mono` for the smaller interface text.

The CSS is built around variables like:

```css
--text-primary
--accent
--surface
--border
--shadow-base
```

Those variables are changed at runtime after the wallpaper is analyzed. This is
how the same page can look dark, light, warm, cold, soft, or neon depending on
the wallpaper.

---

## Fast Loading

The first script that runs is `extension/fastload.js`.

Its job is tiny but important: avoid the ugly blank flash.

When a new tab opens, it immediately checks `localStorage` for:

| Key | Meaning |
|---|---|
| `instantWallpaper` | The last wallpaper image that was successfully loaded. |
| `dynamicPalette` | The last set of CSS colors generated from a wallpaper. |

If those values exist, the page applies them instantly before the heavier work
begins. This makes the new tab feel quick, because the previous wallpaper and
colors appear right away.

After that, the extension still asks the native host for the current wallpaper.
If the wallpaper has changed, the cache is updated.

---

## Wallpaper Loading

The wallpaper pipeline lives in `extension/newtab.js` and `host/main.cpp`.

The extension opens a native messaging connection:

```js
chrome.runtime.connectNative('com.suntzv.pratibimb')
```

Then it sends a simple message:

```js
{ "text": "get_wallpaper" }
```

The message text is not deeply inspected by the native host right now. The host
mainly waits for any valid message, then reads the wallpaper and responds.

### Where Windows Stores The Wallpaper

The native host looks here:

```text
%APPDATA%\Microsoft\Windows\Themes\TranscodedWallpaper
```

That is Windows' own cached copy of the current desktop wallpaper.

### Why The Image Is Compressed

Wallpaper files can be large. Sending a giant raw image through browser native
messaging is slow and fragile, so the host tries to decode the image with
`stb_image.h`, re-encode it as a JPEG using `stb_image_write.h`, and send the
compressed result instead.

The JPEG quality is set to `80`, which is a practical middle ground: it keeps
the image good enough for a full-screen background while making the payload much
smaller.

If STB cannot decode the wallpaper for some reason, the host falls back to
sending the raw file as base64.

### Why The Image Is Sent In Chunks

The native host sends the base64 image in chunks of about 500,000 characters.

Each browser message looks like this:

```json
{
  "chunk": "...part of the image...",
  "done": false
}
```

The final one has:

```json
{
  "chunk": "...last part...",
  "done": true
}
```

The extension keeps adding chunks together until `done` becomes `true`. Then it
creates a browser image URL like this:

```text
data:image/jpeg;base64,...
```

That final image becomes the new tab background.

---

## Color Matching

After the wallpaper arrives, `extension/newtab.js` loads it into a small canvas.

It shrinks the image down to `16 x 16` pixels and studies that tiny version.
That sounds almost too small, but it is enough to understand the mood of the
image without doing heavy work.

The script looks for two things:

| Question | Why it matters |
|---|---|
| Is the lower part of the image light or dark? | Most UI sits near the bottom, so text must stay readable there. |
| What is the most lively color in the image? | That becomes the accent color for glows, focus rings, and highlights. |

If the lower part of the wallpaper is bright, Pratibimb switches to a light UI:
dark text, pale glass, softer shadows.

If the lower part is dark, it switches to a dark UI:
white text, black glass, stronger shadows.

The accent color is chosen from a pixel that is colorful without being too dark
or too washed out. If the image is dull, the script forces the neon accent to
stay vivid enough to read.

The generated CSS is stored in:

```text
localStorage.dynamicPalette
```

That way the next tab can start with the same look instantly.

---

## Clock, Greeting, And Idle Mode

`extension/ui.js` handles the parts of the page that feel alive after the
wallpaper is already in place.

### Clock

The clock updates every second.

Clicking the clock switches between:

- 24-hour time.
- 12-hour time.

The preference is saved in:

```text
localStorage.clock24
```

By default, the extension uses 24-hour time unless `clock24` is explicitly set
to `false`.

### Greeting

The greeting changes based on the time of day:

| Time | Greeting group |
|---|---|
| Before 5 AM | Late night |
| 5 AM to before 12 PM | Morning |
| 12 PM to before 5 PM | Afternoon |
| 5 PM to before 9 PM | Evening |
| 9 PM onward | Night |

Each group has a few short phrases, and one is picked randomly when the period
changes.

### Idle Mode

After 10 seconds without movement, typing, scrolling, clicking, or touch input,
the page enters idle mode.

In idle mode:

- Weather fades out.
- The big clock fades out.
- The date fades out.
- The bottom links fade out.
- The search bar becomes almost invisible.
- A smaller clock appears inside the search bar.

Any user activity brings the full UI back.

---

## Weather

Weather is handled in `extension/ui.js`.

The extension first checks whether it has a recent weather cache:

```text
localStorage.wx_cache
localStorage.wx_time
```

If the cache is less than 10 minutes old, it reuses it.

If not, the extension tries to use browser geolocation. When location is
available:

1. It gets latitude and longitude from the browser.
2. It asks Nominatim for a readable city, town, or village name.
3. It asks Open-Meteo for current weather.
4. It shows the temperature, weather description, city name, and matching SVG
   icon.

If location is denied or unavailable, then the fallback location is the origin of the repo:

```text
Gurugram, India
Latitude: 28.4595
Longitude: 77.0266
```

Weather icons are stored in `extension/icons/` and are named after weather
codes, such as `0.svg`, `61.svg`, or `95.svg`.

---

## Search

The search bar is also in `extension/ui.js`.

It does three jobs:

### 1. Normal Search

If the input is plain text, pressing Enter opens:

```text
https://www.google.com/search?q=...
```

### 2. Website Opening

If the input starts with `http://`, `https://`, or `www.`, it opens the input as
a website instead of a Google search.

For example:

```text
www.github.com
```

becomes:

```text
https://www.github.com
```

### 3. Search Suggestions

After a short 150ms pause while typing, the extension asks Google suggestions
for up to 6 suggestions.

Arrow keys move through the suggestions.
Enter opens the selected suggestion.
Escape closes the suggestion box.

### Small Calculator

The search bar can also solve simple math expressions.

Examples:

```text
12 + 8
10 * (5 - 2)
2^8
```

If the input looks like math, Pratibimb shows the result in the suggestion box.
Clicking the result puts it back into the search field.

Only basic math characters are allowed before the expression is evaluated:

```text
numbers, spaces, +, -, *, /, %, ^, (, ), .
```

That keeps the calculator narrow and avoids treating normal text as code.

---

## Native Host

The native host is `host/pratibimb_host.exe`, built from `host/main.cpp`.

It is a console-style Windows program, but users normally never run it directly.
The browser launches it when the extension connects through Native Messaging.

The host does this:

1. Switches stdin and stdout to binary mode.
2. Waits for a browser message.
3. Reads the current Windows wallpaper file.
4. Compresses the wallpaper to JPEG if possible.
5. Encodes the image as base64.
6. Splits it into chunks.
7. Sends each chunk back to the browser using the Native Messaging format.

### Native Messaging Format

Native Messaging is strict. Each message must be:

1. A 4-byte length.
2. A JSON message of exactly that length.

The host's `sendMessage()` function handles this:

```cpp
uint32_t len = static_cast<uint32_t>(jsonResponse.length());
cout.write(reinterpret_cast<const char*>(&len), 4);
cout.write(jsonResponse.c_str(), len);
cout.flush();
```

If that length prefix is wrong, the browser will not understand the message.

### Debug Log

The host writes a debug log here:

```text
%TEMP%\pratibimb_log.txt
```

This is the first place to look when the wallpaper is not reaching the browser.
The log records whether the host launched, whether it received a message, where
it looked for the wallpaper, and whether chunks were sent.

---

## Native Host Manifest

The browser needs a manifest file to know which local program it may launch.

The installed manifest is created by the installer at:

```text
%LOCALAPPDATA%\Pratibimb\host\com.suntzv.pratibimb.json
```

It looks like this:

```json
{
  "name": "com.suntzv.pratibimb",
  "description": "Pratibimb Native Host",
  "path": "C:\\Users\\...\\AppData\\Local\\Pratibimb\\host\\pratibimb_host.exe",
  "type": "stdio",
  "allowed_origins": [
    "chrome-extension://YOUR_EXTENSION_ID/"
  ]
}
```

The important line is `allowed_origins`. It means only the installed Pratibimb
extension with that exact browser-generated ID is allowed to launch the host.

This is why setup asks you to paste the extension ID.

---

## Installer

The installer is `install/Pratibimb_Setup.exe`, built from
`install/installer.cpp`.

It is a small Win32 desktop app. It has two modes:

- Install
- Uninstall

It supports these Chromium browsers:

| Browser | Registry location used |
|---|---|
| Chrome | `Software\Google\Chrome\NativeMessagingHosts` |
| Brave | `Software\BraveSoftware\Brave-Browser\NativeMessagingHosts` |
| Edge | `Software\Microsoft\Edge\NativeMessagingHosts` |
| Chromium | `Software\Chromium\NativeMessagingHosts` |
| Opera | `Software\Opera Software\Opera Stable\NativeMessagingHosts` |
| Vivaldi | `Software\Vivaldi\Vivaldi\NativeMessagingHosts` |

Chrome, Brave, and Edge are selected by default in the installer UI.

### Install Mode

When you click `Extract & Open Folder`, the installer:

1. Creates this folder:

   ```text
   %LOCALAPPDATA%\Pratibimb
   ```

2. Writes the embedded `extension.zip` there.
3. Extracts it into:

   ```text
   %LOCALAPPDATA%\Pratibimb\extension
   ```

4. Opens the folder so you can drag the extension into the browser's extension
   page.

After the browser gives you an extension ID, you paste it into the installer and
click `Link Native Host`.

Then the installer:

1. Creates:

   ```text
   %LOCALAPPDATA%\Pratibimb\host
   ```

2. Writes the embedded `pratibimb_host.exe` there.
3. Creates the native host manifest JSON.
4. Writes registry entries for the browsers you selected.

The registry value points the browser to the manifest file.

### Uninstall Mode

When you click `Uninstall Everything`, the installer:

1. Deletes the selected browsers' Native Messaging registry entries.
2. Removes:

   ```text
   %LOCALAPPDATA%\Pratibimb
   ```

You still remove the browser extension itself from the browser's extension page.
The installer removes the native files and browser link, not the browser's own
extension record.

---

## Installed Files

After installation, the user's machine should have:

```text
%LOCALAPPDATA%\Pratibimb\
  extension\
    manifest.json
    newtab.html
    fastload.js
    newtab.js
    ui.js
    icons\

  host\
    pratibimb_host.exe
    com.suntzv.pratibimb.json
```

The browser extension folder is loaded manually through Developer Mode.
The host folder is launched automatically by the browser when the extension asks
for it.

---

## What Gets Stored

Pratibimb stores a few things in the browser's local storage:

| Key | Stored by | Purpose |
|---|---|---|
| `instantWallpaper` | `newtab.js` | Last loaded wallpaper as a data URL. |
| `dynamicPalette` | `newtab.js` | Last generated CSS theme. |
| `clock24` | `ui.js` | Whether the clock uses 24-hour time. |
| `wx_cache` | `ui.js` | Last rendered weather HTML. |
| `wx_time` | `ui.js` | When the weather cache was created. |

The native host also writes a debug file:

```text
%TEMP%\pratibimb_log.txt
```

---

## Privacy Notes

Pratibimb is mostly local, but not completely offline.

Local things:

- Your wallpaper is read locally from Windows.
- The wallpaper is sent from the local native host to the local browser
  extension.
- The cached wallpaper and colors stay in the browser's local storage.

Network things:

- Weather uses Open-Meteo.
- Location-to-city lookup uses Nominatim from OpenStreetMap.
- Search suggestions use Google's suggestion endpoint.
- Actual searches go to Google.

One practical detail: while typing in the search box, search suggestion text is
sent to Google after a short delay. If someone wants a fully private search box,
suggestions should be disabled or replaced.

---

## Build And Packaging Notes

There is no build script in this repository right now. The source is arranged
for Windows C++ builds.

One small note for future maintainers: `installer.cpp` contains MSVC-style
`#pragma comment` lines, but the earlier manual build flow for this project used
MinGW-w64/GCC with explicit library flags. In plain English: the code is Windows
C++, but the exact compiler setup is not locked into a script yet.

The packaging order is:

1. Build the native host from `host/main.cpp`.
2. Put the finished host at:

   ```text
   host/pratibimb_host.exe
   ```

3. Zip the `extension/` folder into:

   ```text
   extension.zip
   ```

4. Compile `install/resources.rc` into `install/resources.res`.
5. Build `install/installer.cpp` and link the resources into
   `Pratibimb_Setup.exe`.

If you are using MinGW-w64, the resource step looks like this from inside the
`install/` folder:

```powershell
windres resources.rc -O coff -o resources.res
```

The installer build used by the old documentation looked like this:

```powershell
g++ -std=c++17 -O2 installer.cpp resources.res -o Pratibimb_Setup.exe -mwindows -municode -lcomctl32 -lole32 -ladvapi32 -lshell32 -ldwmapi -luuid -static -static-libgcc -static-libstdc++
```

The resource file embeds two things into the installer:

```rc
IDR_HOST_EXE RCDATA "../host/pratibimb_host.exe"
IDR_EXT_ZIP  RCDATA "../extension.zip"
```

That is how the installer can carry the extension and host inside one `.exe`.

---

## Chromium Security Limits

This project exists in the slightly awkward corner where a browser extension
wants to change the New Tab page and also talk to a local Windows program.
Chromium allows that, but only if the pieces are connected in a very specific
way.

### Why The Extension Is Loaded Manually

Local `.crx` sideloading is heavily restricted, especially for extensions that
replace the New Tab page. The reliable local development/install route is:

1. Extract the `extension` folder.
2. Open the browser extensions page.
3. Turn on Developer Mode.
4. Load or drag in the unpacked folder.

That is why the installer does not silently install the browser extension from
start to finish. The browser wants the user to approve that part.

### Why The Extension ID Matters So Much

When an unpacked extension is loaded from a folder, Chromium creates its ID from
that folder path. If the folder moves, the ID can change.

If the ID changes, the native host manifest still points to the old ID, and the
browser will refuse to launch the host. Nothing is "haunted"; the extension just
needs to be linked again through the installer with the new ID.

---

## Common Problems

### New tab opens, but no wallpaper appears

Check:

- Is the native host registry entry present for your browser?
- Does the installed manifest point to the correct host exe?
- Does `allowed_origins` contain the exact extension ID?
- Does `%TEMP%\pratibimb_log.txt` say the host launched?
- Is Windows Defender blocking `%LOCALAPPDATA%\Pratibimb\host\pratibimb_host.exe`?

### Wallpaper appears only after a delay

This can happen on the very first run because there is no cache yet. After one
successful load, `fastload.js` should reuse `instantWallpaper` and make the next
tab feel immediate.

### Weather does not show the right city

The browser may have denied location permission, or the reverse lookup may have
failed. In that case Pratibimb falls back to Gurugram.

### Search suggestions do not appear

Check whether the browser allows the extension to request:

```text
https://suggestqueries.google.com/*
```

Also check network blocking, DNS blocking, or privacy extensions.

### Native host works in one browser but not another

Each browser has its own Native Messaging registry location. The installer must
write the host path under the browser you actually use.

Also, Store-installed browsers can be more restricted than normal desktop
installers. A direct browser install from the official browser website is safer
for this kind of extension.

### Brave on Windows 11 refuses to cooperate

The README notes that this setup has been painful specifically with Brave on
Windows 11. If Chrome or Edge works but Brave does not, the code may be fine and
the blocker may be browser security behavior.

---

## Files Worth Knowing

| File | Why it matters |
|---|---|
| `extension/manifest.json` | Extension name, version, permissions, and new tab override. |
| `extension/newtab.html` | The page structure and most of the styling. |
| `extension/fastload.js` | Applies cached wallpaper and colors as early as possible. |
| `extension/newtab.js` | Talks to the native host and creates the wallpaper theme. |
| `extension/ui.js` | Clock, weather, search, suggestions, calculator, and idle mode. |
| `host/main.cpp` | Reads, compresses, encodes, and sends the Windows wallpaper. |
| `host/stb_image.h` | Image decoding library used by the host. |
| `host/stb_image_write.h` | JPEG writing library used by the host. |
| `install/installer.cpp` | Installer UI, extraction, registry linking, and uninstall logic. |
| `install/resources.rc` | Tells Windows which files get embedded into the installer. |

---

## A Friendly Mental Model

Think of Pratibimb like a tiny stage crew.

The extension is the stage: lights, clock, search bar, weather, and all the
visible mood.

The native host is the runner who goes backstage into Windows, grabs the current
wallpaper, and hands it to the stage.

The installer is the person who introduces them and says, "You two are allowed
to talk."

Once that relationship is set up, every new tab becomes a fresh reflection of
the desktop.
