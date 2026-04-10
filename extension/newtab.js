let fullImageBase64 = "";

const port = chrome.runtime.connectNative('com.suntzv.pratibimb');

port.onMessage.addListener(function(msg) {
    if (msg.chunk) {
        fullImageBase64 += msg.chunk;
    }
    
    if (msg.done) {
        const finalImageUrl = `data:image/jpeg;base64,${fullImageBase64}`;
        const currentCache = localStorage.getItem('instantWallpaper');
        
        if (currentCache !== finalImageUrl) {
            try {
                localStorage.setItem('instantWallpaper', finalImageUrl);
                document.documentElement.style.backgroundImage = `url('${finalImageUrl}')`;

                const img = new Image();
                img.onload = function() {
                    const canvas = document.createElement('canvas');
                    canvas.width = 1;
                    canvas.height = 1;
                    const ctx = canvas.getContext('2d', { willReadFrequently: true });
                    
                    ctx.drawImage(img, 0, 0, 1, 1);
                    const [r, g, b] = ctx.getImageData(0, 0, 1, 1).data;
                    
                    const colorStr = `rgb(${r}, ${g}, ${b})`;
                    localStorage.setItem('wallpaperColor', colorStr);
                    document.documentElement.style.backgroundColor = colorStr;
                };
                img.src = finalImageUrl;
                
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

port.postMessage({ text: "get_wallpaper" });