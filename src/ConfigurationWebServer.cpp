#include "ConfigurationWebServer.h"
#include "AircraftManager.h"
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <algorithm>

static const char CONFIG_HTML[] PROGMEM = R"RAWHTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0">
<title>Sky Frame — Flight Tracker Remote & Setup</title>
<style>
  :root {
    --bg: #0b111c;
    --card: #121c2e;
    --card-alt: #17243b;
    --border: #253f63;
    --accent: #00e5ff;
    --accent-dim: rgba(0, 229, 255, 0.14);
    --pin: #ff46d2;
    --text: #ebf5ff;
    --muted: #7f9bbd;
    --success: #2ee59d;
    --warn: #ffb900;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    background: var(--bg);
    color: var(--text);
    line-height: 1.45;
    padding-bottom: 40px;
  }
  header {
    background: linear-gradient(135deg, #13223b 0%, #0d1626 100%);
    border-bottom: 1px solid var(--border);
    padding: 16px 18px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    position: sticky;
    top: 0;
    z-index: 20;
  }
  .brand-title { font-size: 1.15rem; font-weight: 700; letter-spacing: 0.04em; color: #fff; }
  .brand-sub { font-size: 0.78rem; color: var(--accent); }
  .pill-live {
    background: var(--accent-dim);
    border: 1px solid var(--accent);
    color: var(--accent);
    padding: 4px 10px;
    border-radius: 999px;
    font-size: 0.75rem;
    font-weight: 600;
  }
  .tabs {
    display: flex;
    max-width: 720px;
    margin: 14px auto 0;
    padding: 0 14px;
    gap: 8px;
  }
  .tab-btn {
    flex: 1;
    background: var(--card);
    border: 1px solid var(--border);
    color: var(--muted);
    padding: 11px 12px;
    border-radius: 10px;
    font-weight: 600;
    font-size: 0.9rem;
    cursor: pointer;
    transition: all 0.15s;
  }
  .tab-btn.active {
    background: var(--accent);
    color: #07101c;
    border-color: var(--accent);
  }
  .container { max-width: 720px; margin: 14px auto; padding: 0 14px; }
  .card {
    background: var(--card);
    border: 1px solid var(--border);
    border-radius: 12px;
    padding: 16px;
    margin-bottom: 14px;
  }
  .card-title {
    font-size: 0.82rem;
    text-transform: uppercase;
    letter-spacing: 0.08em;
    color: var(--accent);
    font-weight: 700;
    margin-bottom: 12px;
    display: flex;
    justify-content: space-between;
    align-items: center;
  }
  .grid-3 { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; }
  .grid-5 { display: grid; grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); gap: 8px; }
  .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
  @media (max-width: 520px) {
    .grid-3 { grid-template-columns: 1fr; }
    .grid-2 { grid-template-columns: 1fr; }
  }
  .choice-btn {
    background: var(--card-alt);
    border: 1px solid var(--border);
    color: var(--text);
    padding: 11px 10px;
    border-radius: 9px;
    font-size: 0.85rem;
    font-weight: 600;
    cursor: pointer;
    text-align: center;
    transition: all 0.15s;
  }
  .choice-btn.active {
    border-color: var(--accent);
    background: var(--accent-dim);
    color: var(--accent);
    box-shadow: 0 0 0 1px var(--accent);
  }
  .swatch-dot {
    display: inline-block;
    width: 10px;
    height: 10px;
    border-radius: 50%;
    margin-right: 6px;
    vertical-align: middle;
  }
  .row-input { display: flex; gap: 8px; align-items: center; }
  input[type="text"], input[type="number"], input[type="password"], select {
    width: 100%;
    background: #0a101b;
    border: 1px solid var(--border);
    color: var(--text);
    padding: 10px 12px;
    border-radius: 8px;
    font-size: 0.92rem;
  }
  input:focus, select:focus { outline: none; border-color: var(--accent); }
  label { display: block; font-size: 0.82rem; color: var(--muted); margin-bottom: 4px; }
  .field { margin-bottom: 12px; }
  .btn {
    background: var(--accent);
    color: #06101c;
    border: none;
    padding: 10px 16px;
    border-radius: 8px;
    font-weight: 700;
    font-size: 0.9rem;
    cursor: pointer;
    white-space: nowrap;
  }
  .btn-secondary {
    background: var(--card-alt);
    color: var(--text);
    border: 1px solid var(--border);
  }
  .btn-pin {
    background: var(--pin);
    color: #fff;
  }
  .flight-list { display: flex; flex-direction: column; gap: 8px; }
  .flight-item {
    background: var(--card-alt);
    border: 1px solid var(--border);
    border-radius: 9px;
    padding: 10px 12px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    cursor: pointer;
    transition: border-color 0.15s;
  }
  .flight-item.selected { border-color: var(--accent); background: var(--accent-dim); }
  .flight-item.pinned { border-color: var(--pin); }
  .f-main { font-weight: 700; font-size: 0.95rem; color: #fff; }
  .f-sub { font-size: 0.78rem; color: var(--muted); }
  .f-right { text-align: right; font-size: 0.82rem; }
  .f-dist { font-weight: 700; color: var(--accent); }
  .badge {
    display: inline-block;
    padding: 2px 6px;
    border-radius: 4px;
    font-size: 0.7rem;
    font-weight: 700;
    margin-left: 6px;
    background: var(--border);
    color: #fff;
  }
  .badge-pin { background: var(--pin); }
  .stats-strip {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 8px;
    text-align: center;
  }
  .stat-box {
    background: var(--card-alt);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 8px;
  }
  .stat-val { font-size: 1.1rem; font-weight: 700; color: var(--accent); }
  .stat-lbl { font-size: 0.72rem; color: var(--muted); }
  .check-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 10px; }
  .check-item {
    display: flex;
    align-items: center;
    gap: 8px;
    background: var(--card-alt);
    padding: 9px 12px;
    border-radius: 8px;
    border: 1px solid var(--border);
    font-size: 0.85rem;
    color: var(--text);
    cursor: pointer;
  }
  .toast {
    position: fixed;
    bottom: 16px;
    left: 50%;
    transform: translateX(-50%);
    background: var(--success);
    color: #04120c;
    padding: 10px 20px;
    border-radius: 999px;
    font-weight: 700;
    font-size: 0.88rem;
    box-shadow: 0 6px 20px rgba(0,0,0,0.45);
    display: none;
    z-index: 50;
  }
  .hidden { display: none !important; }
</style>
</head>
<body>

<header>
  <div>
    <div class="brand-title" id="hdr-title">SKY FRAME</div>
    <div class="brand-sub" id="hdr-sub">AvioFrame Edition · Live Controller</div>
  </div>
  <div class="pill-live" id="hdr-count">0 Airborne</div>
</header>

<div class="tabs">
  <button class="tab-btn active" id="tabBtnRemote" onclick="switchTab('remote')">Live Remote & Flights</button>
  <button class="tab-btn" id="tabBtnSetup" onclick="switchTab('setup')">Setup & Personalize</button>
</div>

<!-- TAB 1: LIVE REMOTE CONTROL -->
<div class="container" id="tab-remote">

  <div class="card">
    <div class="card-title">
      <span>Display View Mode</span>
      <span style="font-size:0.74rem;color:var(--muted)">Switches screen instantly</span>
    </div>
    <div class="grid-3">
      <button class="choice-btn" id="view-radar" onclick="sendControl({view:'radar'})">Radar + Spotter Card</button>
      <button class="choice-btn" id="view-board" onclick="sendControl({view:'board'})">Split-Flap Departure Board</button>
      <button class="choice-btn" id="view-poster" onclick="sendControl({view:'poster'})">AvioFrame Portrait Poster</button>
    </div>
  </div>

  <div class="card">
    <div class="card-title">
      <span>Color Theme</span>
      <button class="btn btn-secondary" style="padding:4px 10px;font-size:0.75rem" onclick="sendControl({toggleStats:'1'})">Toggle Stats Overlay</button>
    </div>
    <div class="grid-5">
      <button class="choice-btn" id="theme-gallery" onclick="sendControl({theme:'gallery'})"><span class="swatch-dot" style="background:#f4f0e8;border:1px solid #c6302c"></span>Gallery Light</button>
      <button class="choice-btn" id="theme-cockpit" onclick="sendControl({theme:'cockpit'})"><span class="swatch-dot" style="background:#00e5ff"></span>Midnight Cockpit</button>
      <button class="choice-btn" id="theme-solari" onclick="sendControl({theme:'solari'})"><span class="swatch-dot" style="background:#ffb220"></span>Split-Flap Amber</button>
      <button class="choice-btn" id="theme-blueprint" onclick="sendControl({theme:'blueprint'})"><span class="swatch-dot" style="background:#1c4e9c;border:1px solid #fff"></span>Blueprint</button>
      <button class="choice-btn" id="theme-radar" onclick="sendControl({theme:'radar'})"><span class="swatch-dot" style="background:#00ff3c"></span>Classic Radar</button>
    </div>
  </div>

  <div class="card">
    <div class="card-title">
      <span>Track a Specific Flight (Partner Flight Pin)</span>
      <span id="pin-status" style="font-size:0.74rem;color:var(--pin)"></span>
    </div>
    <p style="font-size:0.8rem;color:var(--muted);margin-bottom:10px">
      Lock onto any flight worldwide by Flight Number / Callsign (e.g. <b>BA117</b>, <b>BAW117</b>, <b>U28412</b>) or Tail Registration (<b>G-XWBA</b>) — even outside your local radar radius.
    </p>
    <div class="row-input">
      <input type="text" id="pin-input" placeholder="e.g. BA117 or G-XWBA" style="text-transform:uppercase">
      <button class="btn btn-pin" onclick="pinFlight()">Pin Flight</button>
      <button class="btn btn-secondary" onclick="clearPin()">Clear</button>
    </div>
  </div>

  <div class="card">
    <div class="card-title">
      <span>Radar Range & Session Spotter Stats</span>
      <span id="rad-label" style="color:var(--text)">1.00°</span>
    </div>
    <div class="row-input" style="margin-bottom:14px">
      <button class="btn btn-secondary" onclick="sendControl({radiusDelta:'-0.15'})">Zoom In (+)</button>
      <input type="range" id="rad-slider" min="0.1" max="3.0" step="0.05" value="1.0" style="flex:1" onchange="sendControl({radius:this.value})">
      <button class="btn btn-secondary" onclick="sendControl({radiusDelta:'0.15'})">Zoom Out (-)</button>
    </div>
    <div class="stats-strip">
      <div class="stat-box">
        <div class="stat-val" id="st-active">0</div>
        <div class="stat-lbl">In Range Now</div>
      </div>
      <div class="stat-box">
        <div class="stat-val" id="st-total">0</div>
        <div class="stat-lbl">Spotted Today</div>
      </div>
      <div class="stat-box">
        <div class="stat-val" id="st-closest">---</div>
        <div class="stat-lbl" id="st-closest-call">Closest Flyover</div>
      </div>
    </div>
  </div>

  <div class="card">
    <div class="card-title">
      <span>Live Flights Overhead (Tap to Show on Screen)</span>
      <button class="btn btn-secondary" style="padding:4px 10px;font-size:0.75rem" onclick="refreshStatus()">Refresh</button>
    </div>
    <div class="flight-list" id="flight-list">
      <div style="color:var(--muted);font-size:0.85rem;text-align:center;padding:16px">Loading live aircraft...</div>
    </div>
  </div>

</div>

<!-- TAB 2: SETUP & GIFT PERSONALIZATION -->
<div class="container hidden" id="tab-setup">
  <form id="cfgForm" onsubmit="saveConfig(event)">

    <div class="card">
      <div class="card-title">
        <span>1. Easy Home Location Setup</span>
      </div>
      <div class="row-input" style="margin-bottom:12px;flex-wrap:wrap">
        <button type="button" class="btn" onclick="usePhoneGps()">Use My Phone's GPS</button>
        <select id="city-preset" onchange="applyCityPreset(this.value)" style="flex:1;min-width:190px">
          <option value="">Or pick a city / airport preset...</option>
          <option value="51.4700,-0.4543">London Heathrow (LHR)</option>
          <option value="51.1537,-0.1821">London Gatwick (LGW)</option>
          <option value="53.3537,-2.2750">Manchester (MAN)</option>
          <option value="55.9508,-3.3615">Edinburgh (EDI)</option>
          <option value="51.3827,-2.7191">Bristol (BRS)</option>
          <option value="52.4539,-1.7480">Birmingham (BHX)</option>
          <option value="53.4213,-6.2701">Dublin (DUB)</option>
          <option value="49.0097,2.5479">Paris Charles de Gaulle (CDG)</option>
          <option value="52.3105,4.7683">Amsterdam Schiphol (AMS)</option>
          <option value="50.0379,8.5622">Frankfurt (FRA)</option>
          <option value="48.3537,11.7750">Munich (MUC)</option>
          <option value="47.4582,8.5555">Zurich (ZRH)</option>
          <option value="40.4983,-3.5676">Madrid Barajas (MAD)</option>
          <option value="41.2974,2.0833">Barcelona (BCN)</option>
          <option value="25.2532,55.3657">Dubai (DXB)</option>
          <option value="40.6413,-73.7781">New York (JFK)</option>
          <option value="33.9416,-118.4085">Los Angeles (LAX)</option>
          <option value="37.6213,-122.3790">San Francisco (SFO)</option>
          <option value="41.9742,-87.9073">Chicago O'Hare (ORD)</option>
          <option value="33.6407,-84.4277">Atlanta (ATL)</option>
          <option value="43.6777,-79.6248">Toronto Pearson (YYZ)</option>
          <option value="1.3644,103.9915">Singapore Changi (SIN)</option>
          <option value="35.5494,139.7798">Tokyo Haneda (HND)</option>
          <option value="-33.9399,151.1753">Sydney (SYD)</option>
        </select>
      </div>
      <div class="grid-3">
        <div class="field">
          <label>Latitude</label>
          <input type="number" name="latitude" id="f-lat" step="0.000001" min="-90" max="90" required>
        </div>
        <div class="field">
          <label>Longitude</label>
          <input type="number" name="longitude" id="f-lon" step="0.000001" min="-180" max="180" required>
        </div>
        <div class="field">
          <label>Radar Radius (deg)</label>
          <input type="number" name="radius" id="f-rad" step="0.05" min="0.05" max="10.0" required>
        </div>
      </div>
    </div>

    <div class="card">
      <div class="card-title"><span>2. Gift Personalization & Night Schedule</span></div>
      <div class="grid-2">
        <div class="field">
          <label>Frame Header Title</label>
          <input type="text" name="gift-title" id="f-title" maxlength="32" placeholder="e.g. ALEX'S SKY FRAME">
        </div>
        <div class="field">
          <label>Gift Greeting / Welcome Subtitle</label>
          <input type="text" name="gift-subtitle" id="f-subtitle" maxlength="48" placeholder="e.g. Made with love for you ✈">
        </div>
      </div>
      <div class="grid-3">
        <div class="field">
          <label>Timezone Offset (mins from UTC)</label>
          <input type="number" name="tz-offset" id="f-tz" step="15" min="-720" max="840" placeholder="0 (e.g. 60 for BST)">
        </div>
        <div class="field">
          <label>Night Quiet Start Hour (0-23)</label>
          <input type="number" name="night-start" id="f-nstart" min="0" max="23" value="23">
        </div>
        <div class="field">
          <label>Night Quiet End Hour (0-23)</label>
          <input type="number" name="night-end" id="f-nend" min="0" max="23" value="7">
        </div>
      </div>
      <div class="check-grid">
        <label class="check-item"><input type="checkbox" name="night-dim" id="f-ndim"> Auto-sleep display at night</label>
        <label class="check-item"><input type="checkbox" name="auto-cycle-views" id="f-autocycle"> Auto-cycle display views</label>
      </div>
    </div>

    <div class="card">
      <div class="card-title"><span>3. Display & Media Preferences</span></div>
      <div class="grid-3" style="margin-bottom:12px">
        <div class="field">
          <label>Default View</label>
          <select name="view-mode" id="f-view">
            <option value="radar">Radar + Spotter Card</option>
            <option value="board">Split-Flap Departure Board</option>
            <option value="poster">AvioFrame Portrait Poster</option>
          </select>
        </div>
        <div class="field">
          <label>Color Theme</label>
          <select name="theme" id="f-theme">
            <option value="cockpit">Midnight Cockpit</option>
            <option value="gallery">AvioFrame Gallery Light</option>
            <option value="solari">Split-Flap Amber</option>
            <option value="blueprint">Technical Blueprint</option>
            <option value="radar">Classic Radar Green</option>
          </select>
        </div>
        <div class="field">
          <label>Units</label>
          <select name="units" id="f-units">
            <option value="imperial">Imperial (kt, ft, mi)</option>
            <option value="metric">Metric (m/s, m, km)</option>
          </select>
        </div>
      </div>
      <div class="check-grid">
        <label class="check-item"><input type="checkbox" name="media" id="f-media"> Live Plane Photos & Carrier Logos</label>
        <label class="check-item"><input type="checkbox" name="scanline" id="f-scan"> Radar Sweep Line</label>
        <label class="check-item"><input type="checkbox" name="infotext" id="f-info"> Radar Aircraft Labels</label>
        <label class="check-item"><input type="checkbox" name="triangle" id="f-tri"> Directional Plane Vectors</label>
      </div>
    </div>

    <div class="card">
      <div class="card-title"><span>4. Flight Data Providers (Optional)</span></div>
      <div class="field">
        <label>Primary ADS-B Tracking Provider</label>
        <select name="tracking-provider" id="f-provider">
          <option value="adsblol">adsb.lol (Recommended — Free, Keyless, Includes Aircraft Types & Tail Numbers)</option>
          <option value="opensky">OpenSky Network</option>
        </select>
      </div>
      <div class="grid-3">
        <div class="field">
          <label>OpenSky Client ID (Optional)</label>
          <input type="text" name="opensky-id" id="f-osid">
        </div>
        <div class="field">
          <label>OpenSky Client Secret (Optional)</label>
          <input type="password" name="opensky-secret" id="f-ossec">
        </div>
        <div class="field">
          <label>AirLabs API Key (Optional)</label>
          <input type="text" name="airlabs-key" id="f-airlabs">
        </div>
      </div>
    </div>

    <button type="submit" class="btn" style="width:100%;padding:14px;font-size:1rem">Save & Apply Immediately</button>
  </form>
</div>

<div class="toast" id="toast">Updated!</div>

<script>
function showToast(msg) {
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.style.display = 'block';
  setTimeout(() => { t.style.display = 'none'; }, 2400);
}

function switchTab(tab) {
  document.getElementById('tab-remote').classList.toggle('hidden', tab !== 'remote');
  document.getElementById('tab-setup').classList.toggle('hidden', tab !== 'setup');
  document.getElementById('tabBtnRemote').classList.toggle('active', tab === 'remote');
  document.getElementById('tabBtnSetup').classList.toggle('active', tab === 'setup');
}

function sendControl(params) {
  const fd = new FormData();
  for (const k in params) fd.append(k, params[k]);
  fetch('/api/control', { method: 'POST', body: fd })
    .then(r => r.json())
    .then(data => {
      updateStatusUI(data);
      showToast('Applied to display');
    })
    .catch(() => showToast('Connection error'));
}

function pinFlight() {
  const val = document.getElementById('pin-input').value.trim().toUpperCase();
  sendControl({ pin: val });
}

function clearPin() {
  document.getElementById('pin-input').value = '';
  sendControl({ pin: '' });
}

function applyCityPreset(val) {
  if (!val) return;
  const parts = val.split(',');
  if (parts.length === 2) {
    document.getElementById('f-lat').value = parts[0];
    document.getElementById('f-lon').value = parts[1];
    showToast('Coordinates filled — tap Save below');
  }
}

function usePhoneGps() {
  if (!navigator.geolocation) {
    alert('Geolocation is not available in this browser (or requires HTTPS). Use the City / Airport preset dropdown right next to this button!');
    return;
  }
  navigator.geolocation.getCurrentPosition(
    pos => {
      document.getElementById('f-lat').value = pos.coords.latitude.toFixed(5);
      document.getElementById('f-lon').value = pos.coords.longitude.toFixed(5);
      showToast('Phone GPS coordinates captured!');
    },
    err => {
      alert('Browser blocked GPS over local HTTP. Please select your nearest city/airport from the preset dropdown or type coordinates.');
    },
    { timeout: 6000 }
  );
}

function updateStatusUI(s) {
  if (!s) return;
  if (s.giftTitle) document.getElementById('hdr-title').textContent = s.giftTitle;
  if (s.giftSubtitle) document.getElementById('hdr-sub').textContent = s.giftSubtitle;
  document.getElementById('hdr-count').textContent = (s.trackedCount || 0) + ' Airborne';

  ['radar','board','poster'].forEach(v => {
    const el = document.getElementById('view-' + v);
    if (el) el.classList.toggle('active', s.view === v);
  });
  ['gallery','cockpit','solari','blueprint','radar'].forEach(t => {
    const el = document.getElementById('theme-' + t);
    if (el) el.classList.toggle('active', s.theme === t);
  });

  if (typeof s.radius === 'number') {
    document.getElementById('rad-label').textContent = s.radius.toFixed(2) + '°';
    document.getElementById('rad-slider').value = s.radius;
  }

  const pinStatus = document.getElementById('pin-status');
  if (s.pinnedQuery) {
    pinStatus.textContent = '★ Pinned: ' + s.pinnedQuery;
    if (document.activeElement !== document.getElementById('pin-input')) {
      document.getElementById('pin-input').value = s.pinnedQuery;
    }
  } else {
    pinStatus.textContent = '';
  }

  document.getElementById('st-active').textContent = s.trackedCount || 0;
  document.getElementById('st-total').textContent = s.totalSpotted || 0;
  if (s.closestDist && s.closestDist > 0) {
    document.getElementById('st-closest').textContent = s.closestDist.toFixed(1) + ' ' + (s.distUnit || 'km');
    document.getElementById('st-closest-call').textContent = 'Closest (' + (s.closestCallsign || '') + ')';
  }

  const listEl = document.getElementById('flight-list');
  if (!s.flights || s.flights.length === 0) {
    listEl.innerHTML = '<div style="color:var(--muted);font-size:0.85rem;text-align:center;padding:16px">No airborne flights currently in range.</div>';
  } else {
    listEl.innerHTML = s.flights.map(f => {
      const route = (f.dep || f.arr) ? ` · <b>${f.dep || '---'} → ${f.arr || '---'}</b>` : '';
      const pinBadge = f.pinned ? '<span class="badge badge-pin">PINNED</span>' : '';
      const cls = 'flight-item' + (f.selected ? ' selected' : '') + (f.pinned ? ' pinned' : '');
      return `<div class="${cls}" onclick="sendControl({select:'${f.icao}'})">
        <div>
          <div class="f-main">${f.callsign} <span class="badge">${f.type}</span>${pinBadge}</div>
          <div class="f-sub">${f.airline}${f.reg ? ' (' + f.reg + ')' : ''}${route}</div>
        </div>
        <div class="f-right">
          <div class="f-dist">${f.dist} ${s.distUnit || 'km'} ${f.dir}</div>
          <div class="f-sub">${f.alt} ${s.altUnit || 'm'} · ${f.speed} ${s.speedUnit || 'kt'}</div>
        </div>
      </div>`;
    }).join('');
  }
}

function refreshStatus() {
  fetch('/api/status').then(r => r.json()).then(updateStatusUI).catch(() => {});
}

function loadConfig() {
  fetch('/api/config').then(r => r.json()).then(c => {
    document.getElementById('f-lat').value = c.latitude || '';
    document.getElementById('f-lon').value = c.longitude || '';
    document.getElementById('f-rad').value = c.radius || '1.0';
    document.getElementById('f-title').value = c.giftTitle || 'SKY FRAME';
    document.getElementById('f-subtitle').value = c.giftSubtitle || 'Live Aviation Display';
    document.getElementById('f-tz').value = c.tzOffset || '0';
    document.getElementById('f-nstart').value = c.nightStart || '23';
    document.getElementById('f-nend').value = c.nightEnd || '7';
    document.getElementById('f-ndim').checked = c.nightDim !== false && c.nightDim !== 'false';
    document.getElementById('f-autocycle').checked = c.autoCycle === 'true';
    document.getElementById('f-view').value = c.viewMode || 'radar';
    document.getElementById('f-theme').value = c.theme || 'cockpit';
    document.getElementById('f-units').value = c.units || 'imperial';
    document.getElementById('f-media').checked = c.media !== 'false';
    document.getElementById('f-scan').checked = c.scanline !== 'false';
    document.getElementById('f-info').checked = c.infotext !== 'false';
    document.getElementById('f-tri').checked = c.triangle !== 'false';
    document.getElementById('f-provider').value = c.trackingProvider || 'adsblol';
    document.getElementById('f-osid').value = c.openskyId || '';
    document.getElementById('f-ossec').value = c.openskySecret || '';
    document.getElementById('f-airlabs').value = c.airlabsKey || '';
  });
}

function saveConfig(e) {
  e.preventDefault();
  const fd = new FormData(document.getElementById('cfgForm'));
  fetch('/save', { method: 'POST', body: fd })
    .then(r => r.text())
    .then(msg => {
      showToast(msg || 'Saved & applied!');
      refreshStatus();
    });
}

loadConfig();
refreshStatus();
setInterval(refreshStatus, 4000);
</script>
</body>
</html>
)RAWHTML";

void ConfigurationWebServer::Initialise() {
    if (!MDNS.begin("microradar")) {
        Serial.println("[WARN] Failed to start mDNS. Continuing without mDNS...");
    }

    // Serve self-contained mobile remote & setup app
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/html", reinterpret_cast<const uint8_t*>(CONFIG_HTML), sizeof(CONFIG_HTML) - 1);
    });

    // Serve stored configuration JSON
    server.on("/api/config", HTTP_GET, [&](AsyncWebServerRequest* request) {
        prefs.begin("config", true);
        JsonDocument doc;
        doc["latitude"] = prefs.getString("latitude", "");
        doc["longitude"] = prefs.getString("longitude", "");
        doc["radius"] = prefs.getString("radius", "1.0");
        doc["giftTitle"] = prefs.getString("gift-title", "SKY FRAME");
        doc["giftSubtitle"] = prefs.getString("gift-subtitle", "Live Aviation Display");
        doc["tzOffset"] = prefs.getString("tz-offset", "0");
        doc["nightStart"] = prefs.getString("night-start", "23");
        doc["nightEnd"] = prefs.getString("night-end", "7");
        doc["nightDim"] = prefs.getString("night-dim", "false") == "true";
        doc["autoCycle"] = prefs.getString("auto-cycle-views", "false");
        doc["viewMode"] = prefs.getString("view-mode", "radar");
        doc["theme"] = prefs.getString("theme", "cockpit");
        doc["units"] = prefs.getString("units", "imperial");
        doc["media"] = prefs.getString("media", "true");
        doc["scanline"] = prefs.getString("scanline", "true");
        doc["infotext"] = prefs.getString("infotext", "true");
        doc["triangle"] = prefs.getString("triangle", "true");
        doc["trackingProvider"] = prefs.getString("tracking-provider", "adsblol");
        doc["openskyId"] = prefs.getString("opensky-id", "");
        String secret = prefs.getString("opensky-secret", "");
        std::fill(secret.begin(), secret.end(), '*');
        doc["openskySecret"] = secret;
        doc["airlabsKey"] = prefs.getString("airlabs-key", "");
        prefs.end();

        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });

    // Serve live radar & flights status JSON
    server.on("/api/status", HTTP_GET, [&](AsyncWebServerRequest* request) {
        if (aircraftMgr) {
            request->send(200, "application/json", aircraftMgr->BuildStatusJson());
        } else {
            request->send(200, "application/json", "{}");
        }
    });

    // Instant live remote control endpoint (no reboot required)
    server.on("/api/control", HTTP_POST, [&](AsyncWebServerRequest* request) {
        if (aircraftMgr) {
            if (request->hasParam("view", true)) {
                String v = request->getParam("view", true)->value();
                aircraftMgr->SetViewMode(ParseViewMode(v), true);
            }
            if (request->hasParam("theme", true)) {
                String t = request->getParam("theme", true)->value();
                aircraftMgr->SetThemeId(ParseThemeId(t), true);
            }
            if (request->hasParam("radius", true)) {
                double r = request->getParam("radius", true)->value().toDouble();
                if (r > 0.0) aircraftMgr->setRadius(r);
            }
            if (request->hasParam("radiusDelta", true)) {
                double d = request->getParam("radiusDelta", true)->value().toDouble();
                aircraftMgr->adjustRadius(d);
            }
            if (request->hasParam("pin", true)) {
                String p = request->getParam("pin", true)->value();
                aircraftMgr->SetPinnedFlightQuery(p, true);
            }
            if (request->hasParam("select", true)) {
                String icao = request->getParam("select", true)->value();
                if (!icao.isEmpty()) aircraftMgr->SelectFlight(icao);
            }
            if (request->hasParam("toggleStats", true)) {
                aircraftMgr->ToggleStatsModal();
            }
            request->send(200, "application/json", aircraftMgr->BuildStatusJson());
        } else {
            request->send(200, "application/json", "{}");
        }
    });

    // Handle configuration save without requiring a slow reboot
    server.on("/save", HTTP_POST, [&](AsyncWebServerRequest* request) {
        Serial.println("[POST] Saving configuration...");

        auto TrySaveParam = [request, this](const char* paramName) {
            const auto* param = request->getParam(paramName, true);
            if (param == nullptr) return false;
            prefs.putString(paramName, param->value());
            return true;
        };

        prefs.begin("config", false);

        TrySaveParam("latitude");
        TrySaveParam("longitude");
        TrySaveParam("radius");
        TrySaveParam("gift-title");
        TrySaveParam("gift-subtitle");
        TrySaveParam("tz-offset");
        TrySaveParam("night-start");
        TrySaveParam("night-end");
        TrySaveParam("view-mode");
        TrySaveParam("theme");
        TrySaveParam("units");
        TrySaveParam("tracking-provider");
        TrySaveParam("opensky-id");
        TrySaveParam("airlabs-key");

        const auto* secretParam = request->getParam("opensky-secret", true);
        if (secretParam != nullptr) {
            const String& secret = secretParam->value();
            if (secret.indexOf('*') == -1) {
                prefs.putString("opensky-secret", secret);
            }
        }

        prefs.putString("night-dim", request->hasParam("night-dim", true) ? "true" : "false");
        prefs.putString("auto-cycle-views", request->hasParam("auto-cycle-views", true) ? "true" : "false");
        prefs.putString("media", request->hasParam("media", true) ? "true" : "false");
        prefs.putString("scanline", request->hasParam("scanline", true) ? "true" : "false");
        prefs.putString("triangle", request->hasParam("triangle", true) ? "true" : "false");
        prefs.putString("infotext", request->hasParam("infotext", true) ? "true" : "false");

        prefs.end();

        if (aircraftMgr) {
            aircraftMgr->ReloadRuntimeSettings();
        }

        request->send(200, "text/plain", "Settings saved & applied live!");
    });

    server.begin();
}

const String ConfigurationWebServer::GetStoredString(const char* key)
{
    prefs.begin("config", true);
    const String value = prefs.getString(key, "");
    prefs.end();
    return value;
}

void ConfigurationWebServer::SetStoredString(const char* key, String value)
{
    prefs.begin("config", false);
    prefs.putString(key, value);
    prefs.end();
}
