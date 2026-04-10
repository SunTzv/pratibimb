let fullImageBase64 = "";

// Open a continuous stream to the C++ app
const port = chrome.runtime.connectNative('com.suntzv.pratibimb');

// Listen for the chunks coming from C++
port.onMessage.addListener(function(msg) {
    if (msg.chunk) {
        fullImageBase64 += msg.chunk; // Stitch the Base64 pieces together
    }
    
    // When the C++ app says it is finished sending
    if (msg.done) {
        const finalImageUrl = `data:image/jpeg;base64,${fullImageBase64}`;
        const currentCache = localStorage.getItem('instantWallpaper');
        
        if (currentCache !== finalImageUrl) {
            try {
                localStorage.setItem('instantWallpaper', finalImageUrl);
                document.documentElement.style.backgroundImage = `url('${finalImageUrl}')`;
            } catch (e) {
                console.error("Cache save failed:", e);
                document.documentElement.style.backgroundImage = `url('${finalImageUrl}')`;
            }
        }
        port.disconnect(); // Close the pipe
    }
});

port.onDisconnect.addListener(function() {
    if (chrome.runtime.lastError) {
        console.error("Stream closed with error:", chrome.runtime.lastError.message);
    }
});

// Trigger the C++ app
port.postMessage({ text: "get_wallpaper" });