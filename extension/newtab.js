document.addEventListener('DOMContentLoaded', () => {
    // 1. INSTANT LOAD: Grab the last known wallpaper from local storage
    chrome.storage.local.get(['cachedWallpaper'], function(result) {
        if (result.cachedWallpaper) {
            document.body.style.backgroundImage = `url('${result.cachedWallpaper}')`;
        }
    });

    // 2. BACKGROUND CHECK: Ping the C++ app to see if the wallpaper changed
    chrome.runtime.sendNativeMessage(
        'com.suntzv.pratibimb',
        { text: "get_wallpaper" },
        function(response) {
            if (chrome.runtime.lastError) {
                console.error("Native Messaging Error:", chrome.runtime.lastError.message);
                return;
            }

            if (response && response.image) {
                chrome.storage.local.get(['cachedWallpaper'], function(result) {
                    // 3. COMPARE: Only update the screen and storage if it's a new image
                    if (result.cachedWallpaper !== response.image) {
                        document.body.style.backgroundImage = `url('${response.image}')`;
                        chrome.storage.local.set({ cachedWallpaper: response.image });
                    }
                });
            }
        }
    );
});