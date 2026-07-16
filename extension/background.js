// background.js

let rotatePort = null;
let rotateMode = "shuffle";
let fullImageBase64 = "";
let imageMimeType = "image/jpeg";

function updateAlarm() {
    chrome.storage.local.get(['auto_rotate_enabled', 'auto_rotate_interval'], (res) => {
        const enabled = res.auto_rotate_enabled === true || res.auto_rotate_enabled === 'true';
        const interval = parseInt(res.auto_rotate_interval) || 15; // default 15 minutes
        
        chrome.alarms.clear("wallpaper_rotate");
        
        if (enabled) {
            chrome.alarms.create("wallpaper_rotate", {
                delayInMinutes: interval,
                periodInMinutes: interval
            });
            console.log("Wallpaper auto-rotate alarm set for every " + interval + " minutes.");
        } else {
            console.log("Wallpaper auto-rotate is disabled.");
        }
    });
}

// Initial setup
updateAlarm();

// Listen for settings changes
chrome.storage.onChanged.addListener((changes, area) => {
    if (area === 'local') {
        if (changes.auto_rotate_enabled || changes.auto_rotate_interval) {
            updateAlarm();
        }
        if (changes.auto_rotate_mode) {
            rotateMode = changes.auto_rotate_mode.newValue;
        }
    }
});

// Alarm trigger
chrome.alarms.onAlarm.addListener((alarm) => {
    if (alarm.name === "wallpaper_rotate") {
        rotateWallpaper();
    }
});

function rotateWallpaper() {
    chrome.storage.local.get(['auto_rotate_mode', 'last_wallpaper'], (res) => {
        const mode = res.auto_rotate_mode || "shuffle";
        const lastWallpaper = res.last_wallpaper || "";
        
        rotatePort = chrome.runtime.connectNative('com.suntzv.pratibimb');
        
        rotatePort.onMessage.addListener((msg) => {
            if (msg.action === "list_wallpapers" && msg.files) {
                const files = msg.files;
                if (files.length === 0) {
                    rotatePort.disconnect();
                    return;
                }
                
                let nextFile = files[0];
                
                if (mode === "shuffle") {
                    // Pick a random one, preferably not the same as last one if there are >1
                    if (files.length > 1) {
                        let available = files.filter(f => f !== lastWallpaper);
                        if (available.length === 0) available = files;
                        nextFile = available[Math.floor(Math.random() * available.length)];
                    } else {
                        nextFile = files[0];
                    }
                } else if (mode === "sequential") {
                    // Alphabetical order
                    files.sort();
                    const idx = files.indexOf(lastWallpaper);
                    if (idx >= 0 && idx < files.length - 1) {
                        nextFile = files[idx + 1];
                    } else {
                        nextFile = files[0]; // Wrap around to first
                    }
                }
                
                // We don't care about the chunks sent back since this is just a background trigger.
                // The newtab page will fetch the wallpaper properly when opened.
                // However, we want to update the last_wallpaper state.
                chrome.storage.local.set({ last_wallpaper: nextFile });
                
                // Send the command to host to change OS wallpaper
                rotatePort.postMessage({ action: "set_wallpaper", filename: nextFile });
            }
            
            if (msg.chunk !== undefined) {
                fullImageBase64 += msg.chunk;
                if (msg.mime) imageMimeType = msg.mime;
            }
            
            if (msg.done) {
                const finalImageUrl = `data:${imageMimeType};base64,${fullImageBase64}`;
                chrome.storage.local.set({ instantWallpaper: finalImageUrl }, () => {
                    if (rotatePort) {
                        rotatePort.disconnect();
                        rotatePort = null;
                    }
                });
            }
        });
        
        fullImageBase64 = "";
        imageMimeType = "image/jpeg";
        rotatePort.postMessage({ action: "list_wallpapers" });
    });
}
