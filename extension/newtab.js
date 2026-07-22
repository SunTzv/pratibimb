let fullImageBase64 = "";
let imageMimeType = "image/jpeg"; // default

const port = chrome.runtime.connectNative('com.suntzv.pratibimb');



// Native messaging port connects asynchronously to check for desktop wallpaper updates in the background

// 2. Connect native messaging port to check for wallpaper updates
port.onMessage.addListener(function(msg) {
    if (msg.chunk) {
        fullImageBase64 += msg.chunk;
    }
    
    if (msg.mime) {
        imageMimeType = msg.mime;
    }
    
    if (msg.done) {
        const finalImageUrl = `data:${imageMimeType};base64,${fullImageBase64}`;
        const currentCache = localStorage.getItem('instantWallpaper');
        
        if (currentCache !== finalImageUrl || !localStorage.getItem('dynamicPalette')) {
            try {
                extractAndApplyPalette(finalImageUrl, true);
            } catch (e) {
                console.error(e);
                document.documentElement.style.backgroundImage = `url('${finalImageUrl}')`;
            }
        }
        port.disconnect();
    }
});

port.onDisconnect.addListener(function() {
    if (chrome.runtime.lastError) {
        console.error(chrome.runtime.lastError.message);
    }
});

port.postMessage({ action: "get_wallpaper" });