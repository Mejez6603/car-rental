const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0">
  <title>{CAR_ID} Tracker</title>
  <style>
    :root{
      --bg1:#0b1220; --bg2:#111c34;
      --card:rgba(255,255,255,0.06);
      --card-border:rgba(255,255,255,0.10);
      --text:#e7ecf5; --muted:#8896ab;
      --accent:#3aa7ff;
      --good:#33d17a; --warn:#f5a623; --bad:#ef4b4b;
    }
    *{ box-sizing:border-box; }
    body{
      margin:0; min-height:100vh; display:flex; align-items:center; justify-content:center;
      padding:24px 16px;
      background:radial-gradient(circle at 20% 0%, var(--bg2), var(--bg1) 60%);
      font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;
      color:var(--text);
    }
    .card{
      width:100%; max-width:400px;
      background:var(--card);
      border:1px solid var(--card-border);
      border-radius:20px;
      padding:22px;
      backdrop-filter:blur(10px);
      -webkit-backdrop-filter:blur(10px);
      box-shadow:0 20px 40px rgba(0,0,0,0.35);
    }
    .head{ display:flex; align-items:center; justify-content:space-between; margin-bottom:18px; }
    .head h1{ font-size:19px; margin:0; font-weight:600; letter-spacing:.3px; }
    .live{ display:flex; align-items:center; gap:6px; font-size:12px; color:var(--muted); }
    .dot{ width:8px; height:8px; border-radius:50%; background:var(--good); }
    .dot.pulse{ animation:pulse 1.6s ease-in-out infinite; }
    .dot.bad{ background:var(--bad); animation:none; }
    @keyframes pulse{
      0%{ box-shadow:0 0 0 0 rgba(51,209,122,.55); }
      70%{ box-shadow:0 0 0 8px rgba(51,209,122,0); }
      100%{ box-shadow:0 0 0 0 rgba(51,209,122,0); }
    }
    .grid{ display:grid; grid-template-columns:1fr 1fr; gap:10px; margin-bottom:14px; }
    .tile{ background:rgba(255,255,255,0.04); border:1px solid var(--card-border); border-radius:14px; padding:12px; }
    .tile .label{ font-size:11px; color:var(--muted); text-transform:uppercase; letter-spacing:.5px; margin-bottom:4px; }
    .tile .value{ font-size:20px; font-weight:600; }
    .badge{ display:inline-flex; align-items:center; gap:6px; font-size:12px; font-weight:600; padding:5px 10px; border-radius:999px; background:rgba(255,255,255,0.05); }
    .badge .dot{ width:7px; height:7px; }
    .badge.good{ color:var(--good); } .badge.good .dot{ background:var(--good); }
    .badge.warn{ color:var(--warn); } .badge.warn .dot{ background:var(--warn); }
    .badge.bad{ color:var(--bad); } .badge.bad .dot{ background:var(--bad); }
    .badge.muted{ color:var(--muted); } .badge.muted .dot{ background:var(--muted); }
    .coords{ background:rgba(255,255,255,0.04); border:1px solid var(--card-border); border-radius:14px; padding:14px; margin-bottom:14px; }
    .coords .row{ display:flex; justify-content:space-between; font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace; font-size:14px; margin-bottom:4px; }
    .coords .row span:first-child{ color:var(--muted); }
    .maps-btn{ display:block; text-align:center; margin-top:10px; padding:9px; border-radius:10px; background:var(--accent); color:#fff; text-decoration:none; font-size:13px; font-weight:600; }
    .row-line{ display:flex; align-items:center; justify-content:space-between; padding:12px 4px; border-top:1px solid var(--card-border); }
    .row-line .label{ font-size:14px; }
    .row-line .sub{ font-size:11px; color:var(--muted); margin-top:2px; }
    .switch{ position:relative; display:inline-block; width:46px; height:26px; flex-shrink:0; }
    .switch input{ opacity:0; width:0; height:0; }
    .slider{ position:absolute; inset:0; background:#33415580; border-radius:999px; cursor:pointer; transition:.2s; }
    .slider:before{ content:""; position:absolute; width:20px; height:20px; left:3px; top:3px; background:#e7ecf5; border-radius:50%; transition:.2s; }
    input:checked + .slider{ background:var(--accent); }
    input:checked + .slider:before{ transform:translateX(20px); }
    .footer{ text-align:center; margin-top:16px; font-size:11px; color:var(--muted); }
    .cfg-section{ padding-top:14px; border-top:1px solid var(--card-border); margin-top:4px; }
    .cfg-row{ display:flex; gap:8px; }
    .cfg input{
      width:100%; margin-bottom:8px; padding:9px 10px; border-radius:10px;
      border:1px solid var(--card-border); background:rgba(255,255,255,0.04);
      color:var(--text); font-size:13px;
    }
    .cfg input::placeholder{ color:var(--muted); }
    .cfg-row input:last-child{ max-width:90px; }
    .save-btn{ width:100%; padding:9px; border:none; border-radius:10px; background:var(--accent); color:#fff; font-size:13px; font-weight:600; cursor:pointer; }
    .cfg-msg{ text-align:center; font-size:11px; color:var(--good); margin-top:6px; min-height:14px; }
  </style>
</head>
<body>
  <div class="card">
    <div class="head">
      <h1>{CAR_ID}</h1>
      <div class="live"><span class="dot pulse" id="liveDot"></span><span id="liveText">Live</span></div>
    </div>

    <div class="grid">
      <div class="tile">
        <div class="label">GPS</div>
        <div class="value" id="gpsValue">{GPS_STATUS}</div>
      </div>
      <div class="tile">
        <div class="label">Satellites</div>
        <div class="value" id="satsValue">{SATS}</div>
      </div>
    </div>

    <div style="margin-bottom:14px;">
      <span class="badge muted" id="netBadge"><span class="dot"></span><span id="netText">{NET_STATUS}</span></span>
    </div>

    <div class="coords" id="coords" style="display:{SHOW_COORDS};">
      <div class="row"><span>Lat</span><span id="latValue">{LAT}</span></div>
      <div class="row"><span>Lng</span><span id="lngValue">{LNG}</span></div>
      <a class="maps-btn" id="mapsLink" href="https://maps.google.com/?q={LAT},{LNG}" target="_blank" rel="noopener">Open in Maps</a>
    </div>

    <div class="row-line">
      <div>
        <div class="label">Privacy Screen</div>
        <div class="sub" id="privacyText">{PRIVACY_STATE}</div>
      </div>
      <label class="switch">
        <input type="checkbox" id="privacyToggle" {PRIVACY_CHECKED}>
        <span class="slider"></span>
      </label>
    </div>

    <div class="cfg-section">
      <div class="label" style="margin-bottom:10px;">Cloud Sync</div>
      <form class="cfg" id="cfgForm" action="/settings" method="POST">
        <div class="cfg-row">
          <input type="text" name="server" value="{CFG_SERVER}" placeholder="Server">
          <input type="number" name="port" value="{CFG_PORT}" placeholder="Port">
        </div>
        <input type="password" name="auth" placeholder="Auth token (leave blank to keep current)" autocomplete="off">
        <button type="submit" class="save-btn">Save</button>
      </form>
      <div class="cfg-msg" id="cfgMsg"></div>
    </div>

    <div class="footer">Updated <span id="ago">just now</span></div>
  </div>

<script>
(function(){
  var liveDot = document.getElementById('liveDot');
  var liveText = document.getElementById('liveText');
  var gpsValue = document.getElementById('gpsValue');
  var satsValue = document.getElementById('satsValue');
  var netBadge = document.getElementById('netBadge');
  var netText = document.getElementById('netText');
  var coords = document.getElementById('coords');
  var latValue = document.getElementById('latValue');
  var lngValue = document.getElementById('lngValue');
  var mapsLink = document.getElementById('mapsLink');
  var privacyText = document.getElementById('privacyText');
  var privacyToggle = document.getElementById('privacyToggle');
  var ago = document.getElementById('ago');
  var lastFetch = Date.now();
  var toggling = false;

  function netClass(s){
    if (s === 'Connected') return 'good';
    if (s.indexOf('Sync') !== -1 || s.indexOf('Wait') !== -1 || s.indexOf('Reconnect') !== -1) return 'warn';
    if (s === 'Standby') return 'muted';
    return 'bad';
  }

  function setLive(ok){
    liveDot.className = ok ? 'dot pulse' : 'dot bad';
    liveText.textContent = ok ? 'Live' : 'Offline';
  }

  function refresh(){
    fetch('/data', { cache: 'no-store' })
      .then(function(r){ return r.json(); })
      .then(function(d){
        setLive(true);
        lastFetch = Date.now();

        gpsValue.textContent = d.gps;
        satsValue.textContent = d.sats;

        netText.textContent = d.net;
        netBadge.className = 'badge ' + netClass(d.net);

        if (d.valid) {
          coords.style.display = 'block';
          latValue.textContent = d.lat.toFixed(5);
          lngValue.textContent = d.lng.toFixed(5);
          mapsLink.href = 'https://maps.google.com/?q=' + d.lat + ',' + d.lng;
        } else {
          coords.style.display = 'none';
        }

        if (!toggling) {
          privacyToggle.checked = d.privacyOn;
          privacyText.textContent = d.privacyOn ? 'ON (Screen Hidden)' : 'OFF (Screen Active)';
        }
      })
      .catch(function(){ setLive(false); });
  }

  privacyToggle.addEventListener('change', function(){
    toggling = true;
    privacyToggle.disabled = true;
    fetch('/toggle', { method: 'POST' })
      .then(refresh)
      .finally(function(){ privacyToggle.disabled = false; toggling = false; });
  });

  var cfgForm = document.getElementById('cfgForm');
  var cfgMsg = document.getElementById('cfgMsg');
  cfgForm.addEventListener('submit', function(e){
    e.preventDefault();
    var body = new URLSearchParams(new FormData(cfgForm));
    cfgMsg.textContent = 'Saving...';
    fetch('/settings', { method: 'POST', body: body })
      .then(function(){
        cfgMsg.textContent = 'Saved';
        cfgForm.auth.value = '';
        setTimeout(function(){ cfgMsg.textContent = ''; }, 2500);
      })
      .catch(function(){ cfgMsg.textContent = 'Save failed'; });
  });

  setInterval(refresh, 2500);
  setInterval(function(){
    var secs = Math.round((Date.now() - lastFetch) / 1000);
    ago.textContent = secs < 2 ? 'just now' : secs + 's ago';
  }, 1000);

  refresh();
})();
</script>
</body>
</html>
)=====";
