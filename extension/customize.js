// Back Navigation - secure fully qualified local transition
document.getElementById('back-btn').addEventListener('click', () => {
    if (typeof chrome !== 'undefined' && chrome.tabs && chrome.tabs.update) {
        chrome.tabs.update({ url: 'chrome://newtab/' });
    } else {
        window.location.href = chrome.runtime.getURL('newtab.html');
    }
});

// 1. Inputs Selection
const toggle = document.getElementById('clock-toggle');
const headingSel = document.getElementById('sel-heading');
const normalSel = document.getElementById('sel-normal');
const greetingSel = document.getElementById('sel-greeting');

// 2. Initialize from localStorage (fast sync paint)
toggle.checked = localStorage.getItem('clock24') !== 'false';
headingSel.value = localStorage.getItem('font_heading') || 'Jaro';
normalSel.value = localStorage.getItem('font_normal') || 'Satoshi';
greetingSel.value = localStorage.getItem('font_greeting') || 'Tangerine';

function applyStylesLocal() {
    const families = [];
    if (headingSel.value !== 'Satoshi') families.push(headingSel.value);
    if (normalSel.value !== 'Satoshi') families.push(normalSel.value);
    if (greetingSel.value !== 'Satoshi') families.push(greetingSel.value);

    // Clean removal of previous dynamically hot-swapped font elements via IDs
    const oldLink = document.getElementById('dynamic-fonts-link');
    if (oldLink) oldLink.remove();
    const oldStyle = document.getElementById('dynamic-fonts-style');
    if (oldStyle) oldStyle.remove();
    
    const fastloadLink = document.getElementById('fastload-fonts-link');
    if (fastloadLink) fastloadLink.remove();
    const fastloadStyle = document.getElementById('fastload-fonts-style');
    if (fastloadStyle) fastloadStyle.remove();

    if (families.length > 0) {
        const link = document.createElement('link');
        link.id = 'dynamic-fonts-link';
        link.rel = 'stylesheet';
        link.href = `https://fonts.googleapis.com/css2?family=${families.map(f => f.replace(/ /g, '+')).join('&family=')}:wght@300;400;500;700&display=swap`;
        document.head.appendChild(link);
    }

    const style = document.createElement('style');
    style.id = 'dynamic-fonts-style';
    style.textContent = `
        .nt-title { font-family: '${headingSel.value}', sans-serif !important; }
        body, .nt-select, .nt-default-btn, .nt-back-btn, .nt-desc, .nt-section-desc { font-family: '${normalSel.value}', sans-serif !important; }
    `;
    document.head.appendChild(style);
}

// Apply dynamic styling initially
applyStylesLocal();

// 3. Initialize from chrome.storage.local (asynchronous master database sync)
if (typeof chrome !== 'undefined' && chrome.storage && chrome.storage.local) {
    chrome.storage.local.get(['clock24', 'font_heading', 'font_normal', 'font_greeting'], (res) => {
        if (res.clock24 !== undefined) {
            toggle.checked = res.clock24 !== false && res.clock24 !== 'false';
            localStorage.setItem('clock24', toggle.checked);
        }
        if (res.font_heading) {
            headingSel.value = res.font_heading;
            localStorage.setItem('font_heading', res.font_heading);
        }
        if (res.font_normal) {
            normalSel.value = res.font_normal;
            localStorage.setItem('font_normal', res.font_normal);
        }
        if (res.font_greeting) {
            greetingSel.value = res.font_greeting;
            localStorage.setItem('font_greeting', res.font_greeting);
        }
        applyStylesLocal();
    });
}

// 4. Save and Update Functions
function saveSettings() {
    localStorage.setItem('clock24', toggle.checked);
    localStorage.setItem('font_heading', headingSel.value);
    localStorage.setItem('font_normal', normalSel.value);
    localStorage.setItem('font_greeting', greetingSel.value);

    if (typeof chrome !== 'undefined' && chrome.storage && chrome.storage.local) {
        chrome.storage.local.set({
            clock24: toggle.checked,
            font_heading: headingSel.value,
            font_normal: normalSel.value,
            font_greeting: greetingSel.value
        });
    }
    applyStylesLocal();
}

toggle.addEventListener('change', saveSettings);
headingSel.addEventListener('change', saveSettings);
normalSel.addEventListener('change', saveSettings);
greetingSel.addEventListener('change', saveSettings);

// 5. Switch to Default Reset
document.getElementById('default-btn').addEventListener('click', () => {
    toggle.checked = true;
    headingSel.value = 'Jaro';
    normalSel.value = 'Satoshi';
    greetingSel.value = 'Tangerine';
    saveSettings();
});

// 6. Live storage changes listener (in case user modifies clock format directly from homepage)
if (typeof chrome !== 'undefined' && chrome.storage && chrome.storage.onChanged) {
    chrome.storage.onChanged.addListener((changes, area) => {
        if (area === 'local') {
            if (changes.clock24) {
                toggle.checked = changes.clock24.newValue !== false && changes.clock24.newValue !== 'false';
                localStorage.setItem('clock24', toggle.checked);
            }
            if (changes.font_heading) {
                headingSel.value = changes.font_heading.newValue;
                localStorage.setItem('font_heading', headingSel.value);
            }
            if (changes.font_normal) {
                normalSel.value = changes.font_normal.newValue;
                localStorage.setItem('font_normal', normalSel.value);
            }
            if (changes.font_greeting) {
                greetingSel.value = changes.font_greeting.newValue;
                localStorage.setItem('font_greeting', greetingSel.value);
            }
            applyStylesLocal();
        }
    });
}
