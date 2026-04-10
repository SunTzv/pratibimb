document.addEventListener('DOMContentLoaded', () => {
    // Ping the C++ Native Host
    chrome.runtime.sendNativeMessage(
        'com.suntzv.pratibimb',
        { text: "get_wallpaper" },
        function(response) {
            // Check for errors in the connection
            if (chrome.runtime.lastError) {
                console.error("Native Messaging Error:", chrome.runtime.lastError.message);
                return;
            }

            // Apply the image
            if (response && response.image) {
                document.body.style.backgroundImage = `url('${response.image}')`;
            } else {
                console.error("No image data received from Pratibimb Host.");
            }
        }
    );
});