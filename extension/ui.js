/* ── CLOCK ── */
const DAYS   = ['Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday'];
const MONTHS = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];

function tick() {
    const n = new Date();
    const h = String(n.getHours()).padStart(2,'0');
    const m = String(n.getMinutes()).padStart(2,'0');
    document.getElementById('nt-time').innerHTML = `${h}<em>:</em>${m}`;
    document.getElementById('nt-date').textContent =
        `${DAYS[n.getDay()]} · ${MONTHS[n.getMonth()]} ${n.getDate()}, ${n.getFullYear()}`;
    const hr = n.getHours();
    document.getElementById('nt-greeting').textContent =
        hr < 5  ? 'burning the midnight oil.' :
        hr < 12 ? 'good morning.'   :
        hr < 17 ? 'good afternoon.' :
        hr < 21 ? 'good evening.'   : 'good night.';
}
tick(); setInterval(tick, 1000);

/* ── WEATHER ── */
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
    document.getElementById('nt-weather').innerHTML = `
        <div class="nt-wx-row">
            <span class="nt-wx-icon">${icon}</span>
            <span class="nt-wx-temp">${Math.round(temp)}°</span>
        </div>
        <div class="nt-wx-desc">${desc}</div>
        <div class="nt-wx-city">${city}</div>`;
}

function getWeather(lat, lon, city) {
    fetch(`https://api.open-meteo.com/v1/forecast?latitude=${lat}&longitude=${lon}&current_weather=true&temperature_unit=celsius`)
        .then(r => r.json())
        .then(d => paintWeather(d.current_weather.temperature, d.current_weather.weathercode, city))
        .catch(() => {});
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

/* ── SEARCH + AUTOCOMPLETE ── */
const inp  = document.getElementById('nt-search');
const box  = document.getElementById('nt-sugs');
let items = [], cur = -1;

function go(q) {
    if (!q) return;
    window.location.href = /^(https?:\/\/|www\.)/.test(q)
        ? (q.startsWith('http') ? q : 'https://' + q)
        : `https://www.google.com/search?q=${encodeURIComponent(q)}`;
}

function close() { box.classList.remove('open'); box.innerHTML=''; items=[]; cur=-1; }

function paint() {
    if (!items.length) { close(); return; }
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

// Rewritten to comply with Manifest V3 strict CSP limits
function suggest(q) {
    fetch(`https://suggestqueries.google.com/complete/search?client=firefox&q=${encodeURIComponent(q)}`)
        .then(res => res.json())
        .then(data => {
            items = (data[1] || []).slice(0, 6);
            cur = -1;
            paint();
        })
        .catch(() => close());
}

inp.addEventListener('input', () => { const q=inp.value.trim(); q ? suggest(q) : close(); });
inp.addEventListener('keydown', e => {
    if (e.key==='ArrowDown') { e.preventDefault(); cur=Math.min(cur+1,items.length-1); if(cur>=0)inp.value=items[cur]; paint(); }
    else if(e.key==='ArrowUp') { e.preventDefault(); cur=Math.max(cur-1,-1); paint(); }
    else if(e.key==='Enter') { e.preventDefault(); go(cur>=0?items[cur]:inp.value.trim()); }
    else if(e.key==='Escape') close();
});
document.addEventListener('click', e => { if(!document.getElementById('nt-swrap').contains(e.target)) close(); });
inp.focus();
document.addEventListener('keydown', e => {
    if(e.key.length===1 && !e.ctrlKey && !e.metaKey && !e.altKey && document.activeElement!==inp) inp.focus();
});