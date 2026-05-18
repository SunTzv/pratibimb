# DEV_LOG.MD — Project Pratibimb

**Status:** Somehow works.
**Author:** Rishabh Pandey (SunTzv)

What started as “lemme sync my wallpaper to the browser new tab” accidentally became a side quest into browser protocols, Windows weirdness, memory management, and emotional damage.

> The Build

### UI Stuff

The browser tab originally flashed a bright white screen before loading the wallpaper, which felt like getting flashbanged every time I opened a tab.

So I added caching to make everything load almost instantly and spent way too long making the UI look clean, color synced, and actually pleasant instead of “random extension from 2017”.

Also fought browser permissions because Chromium treats every extension like it’s plotting against humanity.

---

### C++ Suffering

Raw wallpaper images are HUGE.

So the app now compresses images before sending them to the browser, because apparently browsers don’t enjoy being force-fed gigantic blobs of data.

Then came:

* memory leaks
* buffer problems
* chunking data into smaller pieces
* wondering why everything breaks when it “should work”

Classic C++ experience.

---

### Deployment Pain

The app worked perfectly on my machine.

Naturally, it exploded everywhere else.

Had to make an installer, fix dependency issues, and package everything so Windows wouldn’t immediately panic and die trying to launch it.

---

### Windows & Browser Boss Fights

This was the real final boss.

Things randomly failed because:

* Windows changed line endings
* the process exited too early
* security settings silently murdered the executable
* Brave and Windows 11 teamed up against me personally

At some point I stopped debugging code and started debugging reality itself.

---

## Edit (1 Month Later)

Came back and worked on it again because apparently I enjoy suffering.

### Added:

* Real icons instead of emojis (we are professionals now)
* Idle mode
  Took around 2-3 hours for something that sounded like a “quick addition”.

---

## Takeaway

Systems programming is basically:

* 20% writing code
* 80% fighting your OS, browser, compiler, and existence

But hey, it works now. Mostly.