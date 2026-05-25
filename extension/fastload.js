// 1. Instantly inject the dynamic UI colors AND the background color (0ms)
const palette = localStorage.getItem('dynamicPalette');
if (palette) {
    document.documentElement.style.cssText += palette;
}

// 2. Instantly start loading the heavy high-res image
const fastCache = localStorage.getItem('instantWallpaper');
if (fastCache) {
    document.documentElement.style.backgroundImage = `url('${fastCache}')`;
}

// 3. Dynamic Font Loader (0ms layout shift, loads Google Fonts synchronously)
window.applyFonts = function(headingF, normalF, greetingF) {
    // Clean old style/link elements if they exist (supporting dynamic hot-swapping)
    const oldLink = document.getElementById('fastload-fonts-link');
    if (oldLink) oldLink.remove();
    const oldStyle = document.getElementById('fastload-fonts-style');
    if (oldStyle) oldStyle.remove();

    const families = [];
    if (headingF !== 'Satoshi') families.push(headingF);
    if (normalF !== 'Satoshi') families.push(normalF);
    if (greetingF !== 'Satoshi') families.push(greetingF);

    if (families.length > 0) {
        const link = document.createElement('link');
        link.id = 'fastload-fonts-link';
        link.rel = 'stylesheet';
        link.href = `https://fonts.googleapis.com/css2?family=${families.map(f => f.replace(/ /g, '+')).join('&family=')}:wght@300;400;500;700&display=swap`;
        document.head.appendChild(link);
    }

    const style = document.createElement('style');
    style.id = 'fastload-fonts-style';
    style.textContent = `
        .nt-time, .nt-wx-temp, .nt-search-idle-clock { font-family: '${headingF}', sans-serif !important; }
        body, .nt-search, .nt-sug, .nt-link, .nt-date, .nt-wx-info-sub { font-family: '${normalF}', sans-serif !important; }
        .nt-greeting { font-family: '${greetingF}', cursive !important; }
    `;
    document.head.appendChild(style);
};

const headingF = localStorage.getItem('font_heading') || 'Jaro';
const normalF = localStorage.getItem('font_normal') || 'Satoshi';
const greetingF = localStorage.getItem('font_greeting') || 'Tangerine';

window.applyFonts(headingF, normalF, greetingF);

// 4. Asynchronously sync from chrome.storage.local to guarantee settings persistence and live sync
if (typeof chrome !== 'undefined' && chrome.storage && chrome.storage.local) {
    chrome.storage.local.get([
        'font_heading',
        'font_normal',
        'font_greeting',
        'clock24',
        'dynamicPalette',
        'instantWallpaper'
    ], (res) => {
        let needsUpdate = false;

        if (res.font_heading !== undefined && res.font_heading !== headingF) needsUpdate = true;
        if (res.font_normal !== undefined && res.font_normal !== normalF) needsUpdate = true;
        if (res.font_greeting !== undefined && res.font_greeting !== greetingF) needsUpdate = true;

        if (res.clock24 !== undefined && String(res.clock24) !== localStorage.getItem('clock24')) {
            localStorage.setItem('clock24', res.clock24);
            if (typeof use24Hour !== 'undefined') {
                use24Hour = res.clock24 !== false && res.clock24 !== 'false';
                if (typeof lastTimeStr !== 'undefined') lastTimeStr = "";
                if (typeof tick === 'function') tick();
            }
        }

        if (res.dynamicPalette !== undefined && res.dynamicPalette !== palette) {
            localStorage.setItem('dynamicPalette', res.dynamicPalette);
            document.documentElement.style.cssText += res.dynamicPalette;
        }

        if (res.instantWallpaper !== undefined && res.instantWallpaper !== fastCache) {
            localStorage.setItem('instantWallpaper', res.instantWallpaper);
            document.documentElement.style.backgroundImage = `url('${res.instantWallpaper}')`;
        }

        if (needsUpdate) {
            const newHeading = res.font_heading || 'Jaro';
            const newNormal = res.font_normal || 'Satoshi';
            const newGreeting = res.font_greeting || 'Tangerine';
            localStorage.setItem('font_heading', newHeading);
            localStorage.setItem('font_normal', newNormal);
            localStorage.setItem('font_greeting', newGreeting);
            window.applyFonts(newHeading, newNormal, newGreeting);

            // If we are on the customize page, also update its select elements dynamically!
            const selHeading = document.getElementById('sel-heading');
            const selNormal = document.getElementById('sel-normal');
            const selGreeting = document.getElementById('sel-greeting');
            if (selHeading && selNormal && selGreeting) {
                selHeading.value = newHeading;
                selNormal.value = newNormal;
                selGreeting.value = newGreeting;
            }
        }
    });
}