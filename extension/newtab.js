// Ping C++ immediately. Do not wait for the page to finish loading.
chrome.runtime.sendNativeMessage(
    'com.suntzv.pratibimb',
    { text: "get_wallpaper" },
    function(response) {
        if (chrome.runtime.lastError) {
            console.error("Native Messaging Error:", chrome.runtime.lastError.message);
            return;
        }

        if (response && response.image) {
            const currentCache = localStorage.getItem('instantWallpaper');
            
            // Compare what we just got from C++ with what is on the screen
            if (currentCache !== response.image) {
                try {
                    // 1. Save it for the next tab (Synchronous, instant load)
                    localStorage.setItem('instantWallpaper', response.image);
                    
                    // 2. Update the current screen to the new image
                    document.documentElement.style.backgroundImage = `url('${response.image}')`;
                    
                } catch (e) {
                    // If the Base64 is truly gigantic, log it but still show the image
                    console.error("Cache save failed:", e);
                    document.documentElement.style.backgroundImage = `url('${response.image}')`;
                }
            }
        }
    }
);