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
                            --text-primary: hsl(${h}, ${s}%, 15%);
                            --text-muted: hsl(${h}, ${Math.max(s-20, 0)}%, 35%);
                            --text-faint: hsl(${h}, ${Math.max(s-30, 0)}%, 50%);
                            --accent: hsl(${h}, ${Math.min(s+40, 100)}%, 35%);
                            --accent-glow: hsla(${h}, ${Math.min(s+40, 100)}%, 35%, 0.18);
                            --surface: hsla(${h}, ${s}%, 95%, 0.35);
                            --surface-hover: hsla(${h}, ${s}%, 95%, 0.55);
                            --border: hsla(${h}, ${s}%, 10%, 0.15);
                            --border-focus: hsla(${h}, ${Math.min(s+40, 100)}%, 35%, 0.45);
                            background-color: rgb(${r}, ${g}, ${b});
                        `;
                    } else {
                        cssVars = `
                            --text-primary: hsl(${h}, ${Math.max(s-10, 0)}%, 95%);
                            --text-muted: hsl(${h}, ${Math.max(s-20, 0)}%, 75%);
                            --text-faint: hsl(${h}, ${Math.max(s-20, 0)}%, 55%);
                            --accent: hsl(${h}, ${Math.min(s+50, 100)}%, 70%);
                            --accent-glow: hsla(${h}, ${Math.min(s+50, 100)}%, 70%, 0.18);
                            --surface: hsla(${h}, ${Math.max(s-20, 0)}%, 5%, 0.35);
                            --surface-hover: hsla(${h}, ${Math.max(s-20, 0)}%, 5%, 0.55);
                            --border: hsla(${h}, ${s}%, 90%, 0.12);
                            --border-focus: hsla(${h}, ${Math.min(s+50, 100)}%, 70%, 0.35);
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