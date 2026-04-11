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
                    
                    // Use a 16x16 grid to calculate BOTH the average brightness and the accent color
                    const canvas = document.createElement('canvas');
                    canvas.width = 16;
                    canvas.height = 16;
                    const ctx = canvas.getContext('2d', { willReadFrequently: true });
                    ctx.drawImage(img, 0, 0, 16, 16);
                    const data = ctx.getImageData(0, 0, 16, 16).data;

                    let maxSat = -1, accentH = 0, accentS = 0;
                    let totalR = 0, totalG = 0, totalB = 0;

                    for (let i = 0; i < data.length; i += 4) {
                        let r = data[i], g = data[i+1], b = data[i+2];
                        totalR += r; totalG += g; totalB += b;

                        let r_n = r/255, g_n = g/255, b_n = b/255;
                        let cmin = Math.min(r_n, g_n, b_n), cmax = Math.max(r_n, g_n, b_n), delta = cmax - cmin;
                        let l = (cmax + cmin) / 2;
                        let s = delta === 0 ? 0 : Math.round(delta / (1 - Math.abs(2 * l - 1)) * 100);

                        if (s > maxSat && l > 0.2 && l < 0.8) {
                            maxSat = s;
                            let h = 0;
                            if (delta !== 0) {
                                if (cmax === r_n) h = ((g_n - b_n) / delta) % 6;
                                else if (cmax === g_n) h = (b_n - r_n) / delta + 2;
                                else h = (r_n - g_n) / delta + 4;
                            }
                            accentH = Math.round(h * 60); if (accentH < 0) accentH += 360;
                            accentS = s;
                        }
                    }

                    // Mathematically perfect average of 256 pixels
                    let avgR = Math.round(totalR / 256);
                    let avgG = Math.round(totalG / 256);
                    let avgB = Math.round(totalB / 256);

                    let brightness = Math.round(((avgR * 299) + (avgG * 587) + (avgB * 114)) / 1000);
                    let isLight = brightness > 128;

                    if (accentS < 20) accentS = 60; // Fallback if image is completely gray

                    let cssVars = "";
                    if (isLight) {
                        // BRIGHT WALLPAPER (Snow): High contrast dark text, visible dark glass panels
                        cssVars = `
                            --text-primary: rgba(0, 0, 0, 0.95);
                            --text-muted: rgba(0, 0, 0, 0.75);
                            --text-faint: rgba(0, 0, 0, 0.55);
                            --text-shadow-color: rgba(255, 255, 255, 0.60);
                            --accent: hsl(${accentH}, ${accentS}%, 30%);
                            --accent-glow: hsla(${accentH}, ${accentS}%, 30%, 0.35);
                            --accent-neon: hsl(${accentH}, ${accentS}%, 70%); /* ALWAYS bright for the dark console */
                            --surface: rgba(255, 255, 255, 0.35); 
                            --surface-hover: rgba(255, 255, 255, 0.55);
                            --border: rgba(0, 0, 0, 0.20);
                            --border-focus: hsla(${accentH}, ${accentS}%, 30%, 0.70);
                            --shadow-base: rgba(0, 0, 0, 0.15);
                            --blur: blur(24px) saturate(120%); 
                            background-color: rgb(${avgR}, ${avgG}, ${avgB});
                        `;
                    } else {
                        // DARK WALLPAPER: Crisp white text, elegant smoked glass
                        cssVars = `
                            --text-primary: rgba(255, 255, 255, 0.95);
                            --text-muted: rgba(255, 255, 255, 0.70);
                            --text-faint: rgba(255, 255, 255, 0.45);
                            --text-shadow-color: rgba(0, 0, 0, 0.60);
                            --accent: hsl(${accentH}, ${accentS}%, 70%);
                            --accent-glow: hsla(${accentH}, ${accentS}%, 70%, 0.35);
                            --accent-neon: hsl(${accentH}, ${accentS}%, 70%); /* Same bright color */
                            --surface: rgba(0, 0, 0, 0.35); 
                            --surface-hover: rgba(0, 0, 0, 0.55);
                            --border: rgba(255, 255, 255, 0.20);
                            --border-focus: hsla(${accentH}, ${accentS}%, 70%, 0.70);
                            --shadow-base: rgba(0, 0, 0, 0.70);
                            --blur: blur(24px) saturate(120%); 
                            background-color: rgb(${avgR}, ${avgG}, ${avgB});
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