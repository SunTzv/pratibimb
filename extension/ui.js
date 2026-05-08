/* ── GREETINGS DATABASE ── */
const GREETINGS = {
    lateNight: [
        "burning the midnight oil.",
        "the quiet hours.",
        "still awake?",
        "the world sleeps."
    ],
    morning: [
        "good morning.",
        "rise and shine.",
        "a fresh start.",
        "let us begin."
    ],
    afternoon: [
        "good afternoon.",
        "steady pace.",
        "keep the momentum.",
        "halfway there."
    ],
    evening: [
        "good evening.",
        "winding down.",
        "twilight hours.",
        "almost done."
    ],
    night: [
        "good night.",
        "time to rest.",
        "lights out.",
        "system standby."
    ]
};

/* ── CLOCK (Optimized DOM Repaints) ── */
const DAYS   = ['Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday'];
const MONTHS = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
let lastTimeStr = "";
let currentPeriod = "";

function getPeriod(hr) {
    if (hr < 5)  return 'lateNight';
    if (hr < 12) return 'morning';
    if (hr < 17) return 'afternoon';
    if (hr < 21) return 'evening';
    return 'night';
}

function tick() {
    const n = new Date();
    const h = String(n.getHours()).padStart(2,'0');
    const m = String(n.getMinutes()).padStart(2,'0');
    const timeStr = `${h}<em>:</em>${m}`;
    
    // [PERF] Only repaint the DOM if the minute actually changed
    if (timeStr !== lastTimeStr) {
        document.getElementById('nt-time').innerHTML = timeStr;
        
        // [NEW] Update the idle clock if it exists
        const idleClock = document.getElementById('nt-idle-clock');
        if (idleClock) idleClock.innerHTML = timeStr;

        document.getElementById('nt-date').textContent = 
            `${DAYS[n.getDay()]} · ${MONTHS[n.getMonth()]} ${n.getDate()}, ${n.getFullYear()}`;
        
        const hr = n.getHours();
        const period = getPeriod(hr);
        
        // Only pick a new random greeting if the time period changed (or on first load)
        if (period !== currentPeriod) {
            const options = GREETINGS[period];
            const randomGreeting = options[Math.floor(Math.random() * options.length)];
            document.getElementById('nt-greeting').textContent = randomGreeting;
            currentPeriod = period;
        }
            
        lastTimeStr = timeStr;
    }
}
tick(); 
setInterval(tick, 1000);

/* ── WEATHER (Aggressive 30-Min Caching) ── */
const WX = {
    0:['☀️','Clear'], 1:['🌤','Mainly clear'], 2:['⛅','Partly cloudy'], 3:['☁️','Overcast'],
    45:['🌫','Fog'], 48:['🌫','Icy fog'],
    51:['🌦','Light drizzle'], 53:['🌦','Drizzle'], 55:['🌧','Heavy drizzle'],
    61:['🌧','Light rain'], 63:['🌧','Rain'], 65:['🌧','Heavy rain'],
    71:['🌨','Light snow'], 73:['🌨','Snow'], 75:['❄️','Heavy snow'],
    80:['🌦','Showers'], 81:['🌧','Rain showers'], 82:['⛈','Violent showers'],
    95:['⛈','Thunderstorm'], 96:['⛈','Hail'], 99:['⛈','Heavy hail'],
};

function paintWeather(temp, code, city) {
    const [icon, desc] = WX[code] || ['🌡','Unknown'];
    const html = `
        <div class="nt-wx-row">
            <span class="nt-wx-icon">${icon}</span>
            <span class="nt-wx-temp">${Math.round(temp)}°</span>
        </div>
        <div class="nt-wx-desc">${desc}</div>
        <div class="nt-wx-city">${city}</div>`;
        
    document.getElementById('nt-weather').innerHTML = html;
    
    // Cache the fully constructed HTML and the timestamp
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
    // [PERF] Load instantly from cache if it's less than 30 minutes old
    const cache = localStorage.getItem('wx_cache');
    const cacheTime = localStorage.getItem('wx_time');
    
    if (cache && cacheTime && (Date.now() - cacheTime < 30 * 60 * 1000)) {
        document.getElementById('nt-weather').innerHTML = cache;
        return;
    }

    if (navigator.geolocation) {
        navigator.geolocation.getCurrentPosition(pos => {
            const {latitude: la, longitude: lo} = pos.coords;
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

/* ── SEARCH + AUTOCOMPLETE (Debounced) ── */
const inp  = document.getElementById('nt-search');
const box  = document.getElementById('nt-sugs');
let items = [], cur = -1;
let debounceTimer; // [PERF] Timer for debouncing

function go(q) {
    if (!q) return;
    window.location.href = /^(https?:\/\/|www\.)/.test(q)
        ? (q.startsWith('http') ? q : 'https://' + q)
        : `https://www.google.com/search?q=${encodeURIComponent(q)}`;
}

function closeSugs() { box.classList.remove('open'); box.innerHTML=''; items=[]; cur=-1; }

function paintSugs() {
    if (!items.length) { closeSugs(); return; }
    box.innerHTML = items.map((s,i) => `
        <div class="nt-sug${i===cur?' hl':''}" data-i="${i}">
            <svg class="nt-sug-ic" width="11" height="11" fill="none" stroke="currentColor" stroke-width="1.8" viewBox="0 0 24 24">
                <circle cx="11" cy="11" r="7"/><line x1="16.5" y1="16.5" x2="22" y2="22"/>
            </svg>${s.replace(/&/g,'&amp;').replace(/</g,'&lt;')}
        </div>`).join('');
    box.classList.add('open');
    box.querySelectorAll('.nt-sug').forEach(el =>
        el.addEventListener('mousedown', e => { e.preventDefault(); go(items[+el.dataset.i]); }));
}

function suggest(q) {
    fetch(`https://suggestqueries.google.com/complete/search?client=firefox&q=${encodeURIComponent(q)}`)
        .then(res => res.json())
        .then(data => {
            items = (data[1] || []).slice(0, 6);
            cur = -1;
            paintSugs();
        })
        .catch(() => closeSugs());
}

// [PERF] Debounce input to prevent API spam while typing fast
inp.addEventListener('input', () => { 
    const q = inp.value.trim(); 
    clearTimeout(debounceTimer);
    if (q) {
        debounceTimer = setTimeout(() => suggest(q), 150); // Waits 150ms before asking Google
    } else {
        closeSugs();
    }
});

inp.addEventListener('keydown', e => {
    if (e.key==='ArrowDown') { e.preventDefault(); cur=Math.min(cur+1,items.length-1); if(cur>=0)inp.value=items[cur]; paintSugs(); }
    else if(e.key==='ArrowUp') { e.preventDefault(); cur=Math.max(cur-1,-1); paintSugs(); }
    else if(e.key==='Enter') { e.preventDefault(); go(cur>=0?items[cur]:inp.value.trim()); }
    else if(e.key==='Escape') closeSugs();
});

document.addEventListener('click', e => { if(!document.getElementById('nt-swrap').contains(e.target)) closeSugs(); });

// [QOL] Smart Focus
inp.focus();
document.addEventListener('keydown', e => {
    if(e.key.length===1 && !e.ctrlKey && !e.metaKey && !e.altKey && document.activeElement!==inp) inp.focus();
});

/* ── INTERNAL LINKS (Bypass Chromium Security Block) ── */
document.querySelectorAll('.nt-link').forEach(link => {
    link.addEventListener('click', e => {
        e.preventDefault(); 
        const url = link.getAttribute('href');
        
        if (chrome && chrome.tabs) {
            chrome.tabs.update({ url: url });
        }
    });
});

/* ── IDLE MODE (10s Inactivity) ── */
const IDLE_TIMEOUT = 10000;
let idleTimer;

// 1. Generate the Idle Clock DOM element & CSS dynamically
const swrap = document.getElementById('nt-swrap');
if (swrap) {
    swrap.style.position = 'relative'; // Required to absolute-position the clock inside it

    // Create the idle clock element
    const idleClock = document.createElement('div');
    idleClock.id = 'nt-idle-clock';
    // Sync the initial time
    const d = new Date();
    idleClock.innerHTML = `${String(d.getHours()).padStart(2,'0')}<em>:</em>${String(d.getMinutes()).padStart(2,'0')}`;
    swrap.appendChild(idleClock);

    // Inject CSS to handle fades automatically
    const idleStyle = document.createElement('style');
    idleStyle.textContent = `
        /* Smooth transitions for elements fading in/out */
        #nt-weather, #nt-time, #nt-date, #nt-greeting, .nt-link, 
        #nt-search, #nt-search::placeholder, #nt-idle-clock {
            transition: opacity 0.5s ease, color 0.5s ease;
        }

        /* Hide all UI elements except the search wrapper */
        body.is-idle #nt-weather,
        body.is-idle #nt-time,
        body.is-idle #nt-date,
        body.is-idle #nt-greeting,
        body.is-idle .nt-link {
            opacity: 0;
            pointer-events: none;
        }

        /* Fade out the search bar's text and placeholder so the clock doesn't overlap it */
        body.is-idle #nt-search {
            color: transparent !important;
        }
        body.is-idle #nt-search::placeholder {
            color: transparent !important;
        }

        /* The Idle Clock styling (Spawns perfectly centered over search bar) */
        #nt-idle-clock {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            font-size: 1.2rem;
            font-weight: 500;
            color: inherit; 
            opacity: 0;
            pointer-events: none; /* Allows clicks to pass through to the search bar */
            z-index: 10;
        }

        /* Show the idle clock when body has the is-idle class */
        body.is-idle #nt-idle-clock {
            opacity: 1;
        }
    `;
    document.head.appendChild(idleStyle);
}

// 2. Idle Timer Logic
function resetIdle() {
    document.body.classList.remove('is-idle');
    clearTimeout(idleTimer);
    
    idleTimer = setTimeout(() => {
        if (typeof closeSugs === 'function') closeSugs(); // Close the autocomplete dropdown if it's open
        document.body.classList.add('is-idle');
    }, IDLE_TIMEOUT);
}

// 3. Listen for any user activity to wake up the UI
['mousemove', 'keydown', 'mousedown', 'wheel', 'touchstart'].forEach(evt => {
    document.addEventListener(evt, resetIdle);
});

// Initialize the timer on load
resetIdle();