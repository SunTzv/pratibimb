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
                    canvas.width = 16;
                    canvas.height = 16;
                    const ctx = canvas.getContext('2d', { willReadFrequently: true });
                    ctx.drawImage(img, 0, 0, 16, 16);
                    const data = ctx.getImageData(0, 0, 16, 16).data;

                    let maxScore = -1000, accentH = 0, accentS = 0;
                    let uiR = 0, uiG = 0, uiB = 0, uiCount = 0;

                    for (let y = 0; y < 16; y++) {
                        for (let x = 0; x < 16; x++) {
                            let i = (y * 16 + x) * 4;
                            let r = data[i], g = data[i+1], b = data[i+2];
                            
                            // ONLY calculate brightness for the bottom half of the screen (where the UI is)
                            if (y >= 6) {
                                uiR += r; uiG += g; uiB += b;
                                uiCount++;
                            }

                            let r_n = r/255, g_n = g/255, b_n = b/255;
                            let cmin = Math.min(r_n, g_n, b_n), cmax = Math.max(r_n, g_n, b_n), delta = cmax - cmin;
                            let l = (cmax + cmin) / 2;
                            let s = delta === 0 ? 0 : Math.round(delta / (1 - Math.abs(2 * l - 1)) * 100);

                            // Find the most VIBRANT color, explicitly rejecting muddy darks and pale whites
                            if (l > 0.25 && l < 0.75) {
                                let l_penalty = Math.abs(0.5 - l) * 100; // Penalize colors that aren't mid-tones
                                let score = s - l_penalty;
                                
                                if (score > maxScore) {
                                    maxScore = score;
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
                        }
                    }

                    // Mathematically perfect average of the UI zone
                    let avgR = Math.round(uiR / uiCount), avgG = Math.round(uiG / uiCount), avgB = Math.round(uiB / uiCount);
                    let brightness = Math.round(((avgR * 299) + (avgG * 587) + (avgB * 114)) / 1000);
                    
                    // Slightly higher threshold to ensure white snow always triggers Light Mode
                    let isLight = brightness > 140; 

                    // Force the neon text to always be vivid and bright, even if the image is dull
                    let neonS = Math.max(accentS, 75); 

                    let cssVars = "";
                    if (isLight) {
                        cssVars = `
                            --text-primary: rgba(0, 0, 0, 0.95);
                            --text-muted: rgba(0, 0, 0, 0.85);
                            --text-faint: rgba(0, 0, 0, 0.65); 
                            --text-shadow-color: rgba(255, 255, 255, 0.90); /* Heavy white outline for legibility */
                            --accent: hsl(${accentH}, ${accentS}%, 30%);
                            --accent-glow: hsla(${accentH}, ${accentS}%, 30%, 0.45);
                            --accent-neon: hsl(${accentH}, ${neonS}%, 45%); /* Deeper neon so it's readable on white */
                            --surface: rgba(255, 255, 255, 0.50); /* Heavier white glass */
                            --surface-hover: rgba(255, 255, 255, 0.75);
                            --border: rgba(0, 0, 0, 0.25); 
                            --border-focus: hsla(${accentH}, ${accentS}%, 30%, 0.80);
                            --shadow-base: rgba(0, 0, 0, 0.15);
                            --blur: blur(28px) saturate(140%); 
                            background-color: rgb(${avgR}, ${avgG}, ${avgB});
                        `;
                    } else {
                        cssVars = `
                            --text-primary: rgba(255, 255, 255, 0.95);
                            --text-muted: rgba(255, 255, 255, 0.75);
                            --text-faint: rgba(255, 255, 255, 0.60);
                            --text-shadow-color: rgba(0, 0, 0, 0.80); /* Heavy black outline */
                            --accent: hsl(${accentH}, ${accentS}%, 70%);
                            --accent-glow: hsla(${accentH}, ${accentS}%, 70%, 0.45);
                            --accent-neon: hsl(${accentH}, ${neonS}%, 70%); /* Bright neon */
                            --surface: rgba(0, 0, 0, 0.35); 
                            --surface-hover: rgba(0, 0, 0, 0.60);
                            --border: rgba(255, 255, 255, 0.20);
                            --border-focus: hsla(${accentH}, ${accentS}%, 70%, 0.80);
                            --shadow-base: rgba(0, 0, 0, 0.70);
                            --blur: blur(28px) saturate(140%); 
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