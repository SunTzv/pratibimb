let fullImageBase64 = "";

const port = chrome.runtime.connectNative('com.suntzv.pratibimb');

function rgbToHsl(r, g, b) {
    r /= 255; g /= 255; b /= 255;
    const max = Math.max(r, g, b), min = Math.min(r, g, b);
    let h = 0, s = 0, l = (max + min) / 2;

    if (max !== min) {
        const d = max - min;
        s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
        switch (max) {
            case r: h = (g - b) / d + (g < b ? 6 : 0); break;
            case g: h = (b - r) / d + 2; break;
            case b: h = (r - g) / d + 4; break;
        }
        h /= 6;
    }
    return [Math.round(h * 360), Math.round(s * 100), Math.round(l * 100)];
}

function extractRegionHsl(data, startX, endX, startY, endY) {
    let uiR = 0, uiG = 0, uiB = 0, uiCount = 0;
    let warmR = 0, warmG = 0, warmB = 0, warmCount = 0;
    let coolR = 0, coolG = 0, coolB = 0, coolCount = 0;

    for (let y = startY; y < endY; y++) {
        for (let x = startX; x < endX; x++) {
            let i = (y * 100 + x) * 4;
            if (i >= data.length) continue;
            let r = data[i], g = data[i+1], b = data[i+2];
            
            uiR += r; uiG += g; uiB += b;
            uiCount++;

            const r_n = r / 255, g_n = g / 255, b_n = b / 255;
            const cmin = Math.min(r_n, g_n, b_n), cmax = Math.max(r_n, g_n, b_n), delta = cmax - cmin;
            const l = (cmax + cmin) / 2;
            const s = delta === 0 ? 0 : Math.round(delta / (1 - Math.abs(2 * l - 1)) * 100);
            const h_val = delta === 0 ? 0 : Math.round((cmax === r_n ? (g_n - b_n) / delta % 6 : cmax === g_n ? (b_n - r_n) / delta + 2 : (r_n - g_n) / delta + 4) * 60);
            const h = h_val < 0 ? h_val + 360 : h_val;

            if (s > 10) {
                if (h < 75 || h > 280) {
                    warmR += r; warmG += g; warmB += b;
                    warmCount++;
                } else {
                    coolR += r; coolG += g; coolB += b;
                    coolCount++;
                }
            }
        }
    }

    if (uiCount === 0) {
        return [0, 0, 50, 0, 0]; // Default fallback if zero pixels scanned
    }

    const avgR = Math.round(uiR / uiCount), avgG = Math.round(uiG / uiCount), avgB = Math.round(uiB / uiCount);
    const [avgH, avgS, avgL] = rgbToHsl(avgR, avgG, avgB);

    let domH, domS;
    if (coolCount > warmCount && coolCount > 0) {
        const [cH, cS] = rgbToHsl(Math.round(coolR / coolCount), Math.round(coolG / coolCount), Math.round(coolB / coolCount));
        domH = cH; domS = cS;
    } else if (warmCount > 0) {
        const [wH, wS] = rgbToHsl(Math.round(warmR / warmCount), Math.round(warmG / warmCount), Math.round(warmB / warmCount));
        domH = wH; domS = wS;
    } else {
        domH = avgH; domS = avgS;
    }

    return [avgH, avgS, avgL, domH, domS];
}

function getRegionStyles(prefix, avgH, avgS, avgL, domH, domS, overallH, overallS) {
    // Prevent erratic green/yellow hue jumps in near-grayscale regions (like white snow / gray rocks)
    // by falling back to the overall dominant wallpaper color signature.
    const isGrayscale = domS < 15;
    const finalH = isGrayscale ? overallH : domH;
    const finalS = isGrayscale ? Math.max(overallS, 40) : domS;
    const neonS = Math.max(finalS, 85);
    
    // Always enforce a premium, dark, highly transparent Liquid Glass container style (no white containers!)
    const textPrimary = `hsla(${finalH}, 88%, 94%, 1.00)`;
    const textMuted = `hsla(${finalH}, 76%, 82%, 0.95)`;
    const textFaint = `hsla(${finalH}, 64%, 68%, 0.90)`;
    const textShadowColor = `hsla(${finalH}, 80%, 4%, 0.92)`;
    const shadowBase = `hsla(${finalH}, 60%, 3%, 0.70)`;
    
    // Translucent dark glass container - highly transparent, Apple Liquid Glass style
    const surface = `hsla(${finalH}, 65%, 8%, 0.40)`;
    const surfaceHover = `hsla(${finalH}, 70%, 12%, 0.50)`;
    const border = `hsla(${finalH}, 40%, 30%, 0.25)`;
    
    // Everything (accent, border focus, hover highlights) is beautifully monochromatic to match
    const accent = `hsl(${finalH}, ${neonS}%, 72%)`;
    const accentGlow = `hsla(${finalH}, ${neonS}%, 72%, 0.45)`;
    const borderFocus = `hsla(${finalH}, ${neonS}%, 72%, 0.75)`;
    const textOnAccent = `hsla(${finalH}, 95%, 4%, 0.98)`;
    
    // Monochromatic hover highlights matching dominant background hue family
    const hoverHighlight = `hsla(${finalH}, ${neonS}%, 70%, 0.85)`;
    const textOnHover = `hsla(${finalH}, 95%, 4%, 0.98)`;
    const hoverHighlightGlow = `hsla(${finalH}, ${neonS}%, 70%, 0.40)`;

    return `
        --${prefix}-text-primary: ${textPrimary};
        --${prefix}-text-muted: ${textMuted};
        --${prefix}-text-faint: ${textFaint};
        --${prefix}-text-shadow-color: ${textShadowColor};
        --${prefix}-accent: ${accent};
        --${prefix}-accent-glow: ${accentGlow};
        --${prefix}-accent-neon: ${accent};
        --${prefix}-surface: ${surface};
        --${prefix}-surface-hover: ${surfaceHover};
        --${prefix}-border: ${border};
        --${prefix}-border-focus: ${borderFocus};
        --${prefix}-shadow-base: ${shadowBase};
        --${prefix}-text-on-accent: ${textOnAccent};
        --${prefix}-hover-highlight: ${hoverHighlight};
        --${prefix}-text-on-hover: ${textOnHover};
        --${prefix}-hover-highlight-glow: ${hoverHighlightGlow};
    `;
}

function extractAndApplyPalette(imageUrl, isNewWallpaper = false) {
    if (!imageUrl) return;
    
    if (isNewWallpaper) {
        localStorage.setItem('instantWallpaper', imageUrl);
        if (typeof chrome !== 'undefined' && chrome.storage && chrome.storage.local) {
            chrome.storage.local.set({ instantWallpaper: imageUrl });
        }
    }
    document.documentElement.style.backgroundImage = `url('${imageUrl}')`;

    const img = new Image();
    img.onload = function() {
        const canvas = document.createElement('canvas');
        canvas.width = 100;
        canvas.height = 100;
        const ctx = canvas.getContext('2d', { willReadFrequently: true });
        ctx.drawImage(img, 0, 0, 100, 100);
        const data = ctx.getImageData(0, 0, 100, 100).data;

        // 1. Extract Global Fallbacks (mapped directly to fallback variables)
        const [overallH, overallS, overallL, domH, domS] = extractRegionHsl(data, 0, 100, 0, 100);
        const globalVars = getRegionStyles('global', overallH, overallS, overallL, domH, domS, overallH, overallS);
        
        const fallbackVars = `
            --text-primary: hsla(${domH}, 88%, 94%, 0.98);
            --text-muted: hsla(${domH}, 76%, 82%, 0.84);
            --text-faint: hsla(${domH}, 64%, 68%, 0.68);
            --text-shadow-color: hsla(${domH}, 80%, 4%, 0.90);
            --accent: hsl(${domH}, ${Math.max(domS, 92)}%, 72%);
            --accent-glow: hsla(${domH}, ${Math.max(domS, 92)}%, 72%, 0.40);
            --accent-neon: hsl(${domH}, ${Math.max(domS, 92)}%, 72%);
            --surface: hsla(${domH}, 55%, 11%, 0.40);
            --surface-hover: hsla(${domH}, 60%, 15%, 0.52);
            --border: hsla(${domH}, 50%, 38%, 0.22);
            --border-focus: hsla(${domH}, ${Math.max(domS, 92)}%, 72%, 0.75);
            --shadow-base: hsla(${domH}, 60%, 3%, 0.65);
            --text-on-accent: rgba(0, 0, 0, 0.95);
            --blur: blur(28px) saturate(140%);
            background-color: rgb(${Math.round(data[0])}, ${Math.round(data[1])}, ${Math.round(data[2])});
        `;

        // 2. Extract regional styles (Apple Liquid Glass segmentations)
        const clockHsl = extractRegionHsl(data, 0, 40, 0, 30);
        const clockVars = getRegionStyles('clock', clockHsl[0], clockHsl[1], clockHsl[2], clockHsl[3], clockHsl[4], overallH, overallS);

        const weatherHsl = extractRegionHsl(data, 60, 100, 0, 30);
        const weatherVars = getRegionStyles('weather', weatherHsl[0], weatherHsl[1], weatherHsl[2], weatherHsl[3], weatherHsl[4], overallH, overallS);

        const searchHsl = extractRegionHsl(data, 20, 80, 35, 65);
        const searchVars = getRegionStyles('search', searchHsl[0], searchHsl[1], searchHsl[2], searchHsl[3], searchHsl[4], overallH, overallS);

        const linksHsl = extractRegionHsl(data, 0, 45, 70, 100);
        const linksVars = getRegionStyles('links', linksHsl[0], linksHsl[1], linksHsl[2], linksHsl[3], linksHsl[4], overallH, overallS);

        const greetingHsl = extractRegionHsl(data, 55, 100, 70, 100);
        const greetingVars = getRegionStyles('greeting', greetingHsl[0], greetingHsl[1], greetingHsl[2], greetingHsl[3], greetingHsl[4], overallH, overallS);

        // 3. Inject all variables
        const cssVars = `
            ${fallbackVars}
            ${globalVars}
            ${clockVars}
            ${weatherVars}
            ${searchVars}
            ${linksVars}
            ${greetingVars}
        `;

        localStorage.setItem('dynamicPalette', cssVars);
        if (typeof chrome !== 'undefined' && chrome.storage && chrome.storage.local) {
            chrome.storage.local.set({ dynamicPalette: cssVars });
        }
        document.documentElement.style.cssText += cssVars;
        
        // Lighter breathing shadow overlays (tinted dynamically with overall background dominant hue)
        document.documentElement.style.setProperty('--idle-overlay-start', `hsla(${domH}, 40%, 3%, 0.08)`);
        document.documentElement.style.setProperty('--idle-overlay-end', `hsla(${domH}, 40%, 3%, 0.18)`);
    };
    img.src = imageUrl;
}

// 1. Immediately extract and apply theme from the cached wallpaper on page load
const cachedWallpaper = localStorage.getItem('instantWallpaper');
if (cachedWallpaper) {
    extractAndApplyPalette(cachedWallpaper, false);
}

// 2. Connect native messaging port to check for wallpaper updates
port.onMessage.addListener(function(msg) {
    if (msg.chunk) {
        fullImageBase64 += msg.chunk;
    }
    
    if (msg.done) {
        const finalImageUrl = `data:image/jpeg;base64,${fullImageBase64}`;
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

port.postMessage({ text: "get_wallpaper" });