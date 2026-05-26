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
const engineSel = document.getElementById('sel-engine');

// Custom Premium Select Building Logic
function initCustomSelects() {
    const selects = [headingSel, normalSel, greetingSel, engineSel];
    
    selects.forEach(select => {
        if (!select) return;
        
        // Create custom outer wrapper
        const wrapper = document.createElement('div');
        wrapper.className = 'nt-select-custom-wrapper';
        wrapper.dataset.selectId = select.id;
        
        // Create custom trigger element
        const trigger = document.createElement('div');
        trigger.className = 'nt-select-trigger';
        
        const triggerText = document.createElement('span');
        triggerText.className = 'nt-select-trigger-text';
        
        const chevron = document.createElement('span');
        chevron.className = 'nt-select-chevron';
        chevron.innerHTML = `
            <svg width="10" height="6" viewBox="0 0 10 6" fill="none" xmlns="http://www.w3.org/2000/svg">
                <path d="M1 1L5 5L9 1" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>
            </svg>
        `;
        
        trigger.appendChild(triggerText);
        trigger.appendChild(chevron);
        wrapper.appendChild(trigger);
        
        // Create dropdown popover list
        const dropdown = document.createElement('div');
        dropdown.className = 'nt-select-dropdown';
        
        // Populate options from the native select options list
        Array.from(select.options).forEach(opt => {
            const optionEl = document.createElement('div');
            optionEl.className = 'nt-select-option';
            optionEl.dataset.value = opt.value;
            optionEl.textContent = opt.textContent;
            
            optionEl.addEventListener('click', (e) => {
                e.stopPropagation();
                select.value = opt.value;
                select.dispatchEvent(new Event('change'));
                wrapper.classList.remove('open');
            });
            
            dropdown.appendChild(optionEl);
        });
        
        wrapper.appendChild(dropdown);
        
        // Insert custom select and hide the native one
        select.parentNode.insertBefore(wrapper, select);
        select.style.display = 'none';
        
        // Toggle menu on click
        trigger.addEventListener('click', (e) => {
            e.stopPropagation();
            document.querySelectorAll('.nt-select-custom-wrapper').forEach(w => {
                if (w !== wrapper) w.classList.remove('open');
            });
            wrapper.classList.toggle('open');
        });
    });
    
    // Close dropdowns when clicking outside
    document.addEventListener('click', () => {
        document.querySelectorAll('.nt-select-custom-wrapper').forEach(w => {
            w.classList.remove('open');
        });
    });
}

function updateCustomDropdowns() {
    const selects = [headingSel, normalSel, greetingSel, engineSel];
    selects.forEach(select => {
        if (!select) return;
        const wrapper = document.querySelector(`.nt-select-custom-wrapper[data-select-id="${select.id}"]`);
        if (!wrapper) return;
        
        const triggerText = wrapper.querySelector('.nt-select-trigger-text');
        const selectedOpt = Array.from(select.options).find(o => o.value === select.value);
        if (selectedOpt && triggerText) {
            triggerText.textContent = selectedOpt.textContent;
        }
        
        wrapper.querySelectorAll('.nt-select-option').forEach(optEl => {
            if (optEl.dataset.value === select.value) {
                optEl.classList.add('selected');
            } else {
                optEl.classList.remove('selected');
            }
        });
    });
}

// Instantiate custom select interfaces
initCustomSelects();

// 2. Initialize from localStorage (fast sync paint)
toggle.checked = localStorage.getItem('clock24') !== 'false';
headingSel.value = localStorage.getItem('font_heading') || 'Jaro';
normalSel.value = localStorage.getItem('font_normal') || 'Satoshi';
greetingSel.value = localStorage.getItem('font_greeting') || 'Tangerine';
engineSel.value = localStorage.getItem('search_engine') || 'google';

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
        body, .nt-select-trigger, .nt-select-option, .nt-default-btn, .nt-back-btn, .nt-desc, .nt-section-desc { font-family: '${normalSel.value}', sans-serif !important; }
    `;
    document.head.appendChild(style);

    // Keep custom dropdown UI fully synced with selected state
    updateCustomDropdowns();
}

// Apply dynamic styling initially
applyStylesLocal();

// 3. Initialize from chrome.storage.local (asynchronous master database sync)
if (typeof chrome !== 'undefined' && chrome.storage && chrome.storage.local) {
    chrome.storage.local.get(['clock24', 'font_heading', 'font_normal', 'font_greeting', 'search_engine'], (res) => {
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
        if (res.search_engine) {
            engineSel.value = res.search_engine;
            localStorage.setItem('search_engine', res.search_engine);
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
    localStorage.setItem('search_engine', engineSel.value);

    if (typeof chrome !== 'undefined' && chrome.storage && chrome.storage.local) {
        chrome.storage.local.set({
            clock24: toggle.checked,
            font_heading: headingSel.value,
            font_normal: normalSel.value,
            font_greeting: greetingSel.value,
            search_engine: engineSel.value
        });
    }
    applyStylesLocal();
}

toggle.addEventListener('change', saveSettings);
headingSel.addEventListener('change', saveSettings);
normalSel.addEventListener('change', saveSettings);
greetingSel.addEventListener('change', saveSettings);
engineSel.addEventListener('change', saveSettings);

// 5. Switch to Default Reset
document.getElementById('default-btn').addEventListener('click', () => {
    toggle.checked = true;
    headingSel.value = 'Jaro';
    normalSel.value = 'Satoshi';
    greetingSel.value = 'Tangerine';
    engineSel.value = 'google';
    saveSettings();
});

// 6. Live storage changes listener (in case user modifies settings directly from homepage)
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
            if (changes.search_engine) {
                engineSel.value = changes.search_engine.newValue;
                localStorage.setItem('search_engine', engineSel.value);
            }
            applyStylesLocal();
        }
    });
}
