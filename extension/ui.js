const GREETINGS = {
    lateNight: ["dead and alive!", "the quiet hours.", "still awake?", "the world sleeps."],
    morning:   ["good morning.", "rise and shine.", "a fresh start.", "let us begin."],
    afternoon: ["good afternoon.", "steady pace.", "keep the momentum.", "halfway there."],
    evening:   ["good evening.", "winding down.", "twilight hours.", "almost done."],
    night:     ["good night.", "time to rest.", "lights out.", "system standby."]
};

const DAYS   = ['Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday'];
const MONTHS = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];

let lastTimeStr    = "";
let currentPeriod  = "";
let use24Hour      = localStorage.getItem('clock24') !== 'false';

// Async master database sync for clock format
if (typeof chrome !== 'undefined' && chrome.storage && chrome.storage.local) {
    chrome.storage.local.get(['clock24'], (res) => {
        if (res.clock24 !== undefined) {
            use24Hour = res.clock24 !== false && res.clock24 !== 'false';
            localStorage.setItem('clock24', use24Hour);
            lastTimeStr = "";
            tick();
        }
    });
}

function getPeriod(hr) {
    if (hr < 5)  return 'lateNight';
    if (hr < 12) return 'morning';
    if (hr < 17) return 'afternoon';
    if (hr < 21) return 'evening';
    return 'night';
}

function formatTime(n) {
    if (use24Hour) {
        const h = String(n.getHours()).padStart(2, '0');
        const m = String(n.getMinutes()).padStart(2, '0');
        return { display: `${h}<em>:</em>${m}`, suffix: '' };
    } else {
        let h = n.getHours();
        const m = String(n.getMinutes()).padStart(2, '0');
        const ampm = h >= 12 ? 'PM' : 'AM';
        h = h % 12 || 12;
        return { display: `${h}<em>:</em>${m}`, suffix: ampm };
    }
}

function tick() {
    const n = new Date();
    const { display, suffix } = formatTime(n);
    const timeStr = display + suffix;

    if (timeStr !== lastTimeStr) {
        const timeEl = document.getElementById('nt-time');
        timeEl.innerHTML = use24Hour
            ? display
            : `${display}<span class="nt-time-ampm">${suffix}</span>`;

        const idleClock = document.getElementById('nt-idle-clock');
        if (idleClock) idleClock.innerHTML = use24Hour
            ? display
            : `${display}<span style="font-size:0.45em;letter-spacing:0.08em;margin-left:4px">${suffix}</span>`;

        document.getElementById('nt-date').textContent =
            `${DAYS[n.getDay()]} · ${MONTHS[n.getMonth()]} ${n.getDate()}, ${n.getFullYear()}`;

        const period = getPeriod(n.getHours());
        if (period !== currentPeriod) {
            const opts = GREETINGS[period];
            document.getElementById('nt-greeting').textContent = opts[Math.floor(Math.random() * opts.length)];
            currentPeriod = period;
        }

        lastTimeStr = timeStr;
    }
}

tick();
setInterval(tick, 1000);

document.getElementById('nt-time').addEventListener('click', () => {
    use24Hour = !use24Hour;
    localStorage.setItem('clock24', use24Hour);
    if (typeof chrome !== 'undefined' && chrome.storage && chrome.storage.local) {
        chrome.storage.local.set({ clock24: use24Hour });
    }
    lastTimeStr = "";
    tick();
});

// Live synchronization between settings changes (fonts/clock) across pages
if (typeof chrome !== 'undefined' && chrome.storage && chrome.storage.onChanged) {
    chrome.storage.onChanged.addListener((changes, area) => {
        if (area === 'local') {
            if (changes.clock24) {
                use24Hour = changes.clock24.newValue !== false && changes.clock24.newValue !== 'false';
                localStorage.setItem('clock24', use24Hour);
                lastTimeStr = "";
                tick();
            }
            if (changes.font_heading || changes.font_normal || changes.font_greeting) {
                const h = (changes.font_heading ? changes.font_heading.newValue : null) || localStorage.getItem('font_heading') || 'Jaro';
                const n = (changes.font_normal ? changes.font_normal.newValue : null) || localStorage.getItem('font_normal') || 'Satoshi';
                const g = (changes.font_greeting ? changes.font_greeting.newValue : null) || localStorage.getItem('font_greeting') || 'Tangerine';
                if (typeof window.applyFonts === 'function') {
                    window.applyFonts(h, n, g);
                }
            }
        }
    });
}

const WX = {
    0: 'Clear', 1: 'Mainly clear', 2: 'Partly cloudy', 3: 'Overcast',
    45: 'Fog', 48: 'Icy fog',
    51: 'Light drizzle', 53: 'Drizzle', 55: 'Heavy drizzle',
    61: 'Light rain', 63: 'Rain', 65: 'Heavy rain',
    71: 'Light snow', 73: 'Snow', 75: 'Heavy snow',
    80: 'Showers', 81: 'Rain showers', 82: 'Violent showers',
    95: 'Thunderstorm', 96: 'Hail', 99: 'Heavy hail',
};

function paintWeather(temp, code, city) {
    const desc    = WX[code] || 'Unknown';
    const iconUrl = chrome.runtime.getURL(`icons/${WX[code] ? code : 0}.svg`);
    const html    = `
        <div class="nt-wx-row">
            <div class="nt-wx-icon" style="-webkit-mask-image: url('${iconUrl}');"></div>
            <span class="nt-wx-temp">${Math.round(temp)}°</span>
        </div>
        <div class="nt-wx-info-sub">${desc} · ${city}</div>`;

    document.getElementById('nt-weather').innerHTML = html;
    localStorage.setItem('wx_cache', html);
    localStorage.setItem('wx_time', Date.now());
}

function getWeather(lat, lon, city) {
    fetch(`https://api.open-meteo.com/v1/forecast?latitude=${lat}&longitude=${lon}&current_weather=true&temperature_unit=celsius`)
        .then(r => r.json())
        .then(d => paintWeather(d.current_weather.temperature, d.current_weather.weathercode, city))
        .catch(() => {});
}

function initWeather() {
    const cache     = localStorage.getItem('wx_cache');
    const cacheTime = localStorage.getItem('wx_time');

    if (cache && cacheTime && (Date.now() - cacheTime < 10 * 60 * 1000) && cache.includes('nt-wx-info-sub')) {
        document.getElementById('nt-weather').innerHTML = cache;
        return;
    }

    if (navigator.geolocation) {
        navigator.geolocation.getCurrentPosition(pos => {
            const { latitude: la, longitude: lo } = pos.coords;
            fetch(`https://nominatim.openstreetmap.org/reverse?lat=${la}&lon=${lo}&format=json`)
                .then(r => r.json())
                .then(d => getWeather(la, lo, d.address.city || d.address.town || d.address.village || 'here'))
                .catch(() => getWeather(la, lo, 'here'));
        }, () => getWeather(28.4595, 77.0266, 'Gurugram'));
    } else {
        getWeather(28.4595, 77.0266, 'Gurugram');
    }
}

initWeather();


const MATH_PATTERN = /^[\d\s\+\-\*\/\%\^\(\)\.]+$/;
const MATH_HAS_OP  = /[\+\-\*\/\%\^]/;

function tryMath(expr) {
    const clean = expr.trim().replace(/\^/g, '**');
    if (!MATH_PATTERN.test(clean) || !MATH_HAS_OP.test(clean)) return null;
    try {
        const result = Function('"use strict"; return (' + clean + ')')();
        if (typeof result !== 'number' || !isFinite(result)) return null;
        const rounded = parseFloat(result.toPrecision(12));
        return String(rounded);
    } catch {
        return null;
    }
}

const inp = document.getElementById('nt-search');
const box = document.getElementById('nt-sugs');
let items = [], cur = -1, debounceTimer;

function go(q) {
    if (!q) return;
    window.location.href = /^(https?:\/\/|www\.)/.test(q)
        ? (q.startsWith('http') ? q : 'https://' + q)
        : `https://www.google.com/search?q=${encodeURIComponent(q)}`;
}

function closeSugs() { box.classList.remove('open'); box.innerHTML = ''; items = []; cur = -1; }

function paintSugs() {
    if (!items.length) { closeSugs(); return; }
    box.innerHTML = items.map((s, i) => `
        <div class="nt-sug${i === cur ? ' hl' : ''}" data-i="${i}">
            <svg class="nt-sug-ic" width="11" height="11" fill="none" stroke="currentColor" stroke-width="1.8" viewBox="0 0 24 24">
                <circle cx="11" cy="11" r="7"/><line x1="16.5" y1="16.5" x2="22" y2="22"/>
            </svg>${s.replace(/&/g, '&amp;').replace(/</g, '&lt;')}
        </div>`).join('');
    box.classList.add('open');
    box.querySelectorAll('.nt-sug').forEach(el =>
        el.addEventListener('mousedown', e => { e.preventDefault(); go(items[+el.dataset.i]); }));
}

function paintMathResult(expr, result) {
    box.innerHTML = `
        <div class="nt-sug nt-sug-math" data-result="${result}">
            <svg class="nt-sug-ic" width="11" height="11" fill="none" stroke="currentColor" stroke-width="1.8" viewBox="0 0 24 24">
                <line x1="12" y1="5" x2="12" y2="19"/><line x1="5" y1="12" x2="19" y2="12"/>
            </svg>${expr.trim()} = <strong>${result}</strong>
        </div>`;
    box.classList.add('open');
    box.querySelector('.nt-sug-math').addEventListener('mousedown', e => {
        e.preventDefault();
        inp.value = result;
        closeSugs();
    });
}

function suggest(q) {
    const mathResult = tryMath(q);
    if (mathResult !== null) {
        items = [];
        cur   = -1;
        paintMathResult(q, mathResult);
        return;
    }

    fetch(`https://suggestqueries.google.com/complete/search?client=firefox&q=${encodeURIComponent(q)}`)
        .then(res => res.json())
        .then(data => {
            items = (data[1] || []).slice(0, 6);
            cur   = -1;
            paintSugs();
        })
        .catch(() => closeSugs());
}

inp.addEventListener('input', () => {
    const q = inp.value.trim();
    clearTimeout(debounceTimer);
    if (q) {
        debounceTimer = setTimeout(() => suggest(q), 150);
    } else {
        closeSugs();
    }
});

inp.addEventListener('keydown', e => {
    if      (e.key === 'ArrowDown')  { e.preventDefault(); cur = Math.min(cur + 1, items.length - 1); if (cur >= 0) inp.value = items[cur]; paintSugs(); }
    else if (e.key === 'ArrowUp')    { e.preventDefault(); cur = Math.max(cur - 1, -1); paintSugs(); }
    else if (e.key === 'Enter')      { e.preventDefault(); go(cur >= 0 ? items[cur] : inp.value.trim()); }
    else if (e.key === 'Escape')     closeSugs();
});

document.addEventListener('click', e => {
    if (!document.getElementById('nt-swrap').contains(e.target)) closeSugs();
});

inp.focus();
document.addEventListener('keydown', e => {
    // Ctrl + I instantly activates Idle Mode on command
    if (e.ctrlKey && e.key.toLowerCase() === 'i') {
        e.preventDefault();
        e.stopImmediatePropagation();
        enterIdleMode();
        return;
    }
    if (e.key.length === 1 && !e.ctrlKey && !e.metaKey && !e.altKey && document.activeElement !== inp) inp.focus();
});


document.querySelectorAll('.nt-link').forEach(link => {
    link.addEventListener('click', e => {
        e.preventDefault();
        let url = link.getAttribute('href');
        if (!url.includes('://')) {
            url = chrome.runtime.getURL(url);
        }
        if (chrome && chrome.tabs) chrome.tabs.update({ url });
    });
});


const IDLE_TIMEOUT = 10000;
let idleTimer;
let mouseMoveStartTime = 0;
let mouseMoveDebounce = null;

function enterIdleMode() {
    if (typeof closeSugs === 'function') closeSugs();
    document.body.classList.add('is-idle');
    clearTimeout(idleTimer);
}

function resetIdle() {
    document.body.classList.remove('is-idle');
    clearTimeout(idleTimer);
    idleTimer = setTimeout(() => {
        if (typeof closeSugs === 'function') closeSugs();
        document.body.classList.add('is-idle');
    }, IDLE_TIMEOUT);
}

function onUserActivity(e) {
    const isIdle = document.body.classList.contains('is-idle');

    if (isIdle) {
        if (e.type === 'keydown') {
            const isAlphanumeric = /^[a-zA-Z0-9]$/.test(e.key);
            if (!isAlphanumeric) {
                return; // Ignore system/modifier keys when in idle mode
            }
        } else if (e.type === 'mousemove') {
            if (!mouseMoveStartTime) {
                mouseMoveStartTime = Date.now();
            }
            
            clearTimeout(mouseMoveDebounce);
            mouseMoveDebounce = setTimeout(() => {
                mouseMoveStartTime = 0; // Reset tracking if mouse movement stops
            }, 150);

            if (Date.now() - mouseMoveStartTime < 300) {
                return; // Ignore quick nudges (must move for >= 300ms)
            }
            
            mouseMoveStartTime = 0;
        }
    } else {
        mouseMoveStartTime = 0;
    }

    resetIdle();
}

['mousemove', 'keydown', 'mousedown', 'wheel', 'touchstart'].forEach(evt => {
    document.addEventListener(evt, onUserActivity);
});

resetIdle();

// Disable right-click context menu
document.addEventListener('contextmenu', e => e.preventDefault());