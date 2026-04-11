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

                    const brightness = Math.round(((r * 299) + (g * 587) + (b * 114)) / 1000);
                    const isLight = brightness > 128;

                    let r_n = r/255, g_n = g/255, b_n = b/255;
                    let cmin = Math.min(r_n, g_n, b_n), cmax = Math.max(r_n, g_n, b_n), delta = cmax - cmin;
                    let h = 0, s = 0, l = (cmax + cmin) / 2;
                    if(delta === 0) h = 0;
                    else if(cmax === r_n) h = ((g_n - b_n) / delta) % 6;
                    else if(cmax === g_n) h = (b_n - r_n) / delta + 2;
                    else h = (r_n - g_n) / delta + 4;
                    h = Math.round(h * 60); if(h < 0) h += 360;
                    s = delta === 0 ? 0 : Math.round(delta / (1 - Math.abs(2 * l - 1)) * 100);
                    l = Math.round(l * 100);

                    let cssVars = "";
                    if (isLight) {
                        cssVars = `
                            --text-primary: hsl(${h}, 10%, 10%);
                            --text-muted: hsl(${h}, 10%, 35%);
                            --text-faint: hsl(${h}, 10%, 50%);
                            --accent: hsl(${h}, 90%, 35%);
                            --accent-glow: hsla(${h}, 90%, 35%, 0.25);
                            --surface: hsla(${h}, 10%, 95%, 0.65); /* 65% opacity for readability */
                            --surface-hover: hsla(${h}, 10%, 100%, 0.85);
                            --border: hsla(${h}, 20%, 0%, 0.15);
                            --border-focus: hsla(${h}, 90%, 35%, 0.50);
                            background-color: rgb(${r}, ${g}, ${b});
                        `;
                    } else {
                        cssVars = `
                            --text-primary: hsl(${h}, 10%, 98%);
                            --text-muted: hsl(${h}, 10%, 75%);
                            --text-faint: hsl(${h}, 10%, 60%);
                            --accent: hsl(${h}, 90%, 75%);
                            --accent-glow: hsla(${h}, 90%, 75%, 0.25);
                            --surface: hsla(${h}, 15%, 5%, 0.65); /* 65% opacity for readability */
                            --surface-hover: hsla(${h}, 15%, 10%, 0.85);
                            --border: hsla(${h}, 20%, 100%, 0.15);
                            --border-focus: hsla(${h}, 90%, 75%, 0.50);
                            background-color: rgb(${r}, ${g}, ${b});
                        `;
                    }

                    localStorage.setItem('dynamicPalette', cssVars);
                    document.documentElement.style.cssText += cssVars;
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