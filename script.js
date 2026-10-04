const blynkToken = "6fub_AeSZfywBab9j-d7KRXWKFPMwIxz";
const blynkServer = "sgp1.blynk.cloud";
// Car-002 shares the same Blynk token/device as Car-001, distinguished by
// virtual pins instead: Car-001 = v1-v4, Car-002 = v5-v8.

const cartoApiKey = "cb1_3p3v_1_c7e75ca303dcbd4b49651900";

const map = L.map('map').setView([14.2600, 121.3958], 16);
L.tileLayer(`https://basemaps.cartocdn.com/rastertiles/voyager/{z}/{x}/{y}.png?key=${cartoApiKey}`, {
    attribution: '&copy; <a href="https://carto.com/attributions">CARTO</a>'
}).addTo(map);

// --- YI TRACKER STYLE CUSTOM PINS ---
function createYiPinIcon(carId, label, isCar2 = false) {
    const carClass = isCar2 ? 'car-2' : '';
    const imgFilter = isCar2 ? 'style="filter:hue-rotate(210deg) saturate(1.3);"' : '';
    const html = `
        <div class="yi-tracker-pin" onclick="onPinClicked('${carId}')">
            <div class="yi-pin-bubble ${carClass}">
                <img src="https://cdn-icons-png.flaticon.com/512/744/744465.png" alt="${label}" ${imgFilter}>
            </div>
            <div class="yi-pin-pointer ${carClass}"></div>
            <div class="yi-pin-badge ${carClass}">${label}</div>
        </div>
    `;
    return L.divIcon({
        html: html,
        className: 'yi-custom-div-icon',
        iconSize: [90, 75],
        iconAnchor: [45, 75],
        popupAnchor: [0, -78]
    });
}

function onPinClicked(carId) {
    if (carId === 'car1') {
        const panel = document.getElementById("details-panel");
        panel.style.display = "block";
        document.getElementById("details-panel-2").style.display = "none";
    } else if (carId === 'car2') {
        const panel = document.getElementById("details-panel-2");
        panel.style.display = "block";
        document.getElementById("details-panel").style.display = "none";
    }
}

function updateMarkerPopup(marker, carName, lat, lng, sats) {
    const content = `
        <div style="font-family:'Segoe UI',sans-serif;min-width:180px;padding:3px;">
            <div style="font-weight:800;font-size:0.95rem;color:#0f172a;margin-bottom:5px;display:flex;align-items:center;gap:6px;">
                <span style="width:8px;height:8px;border-radius:50%;background:#00B14F;display:inline-block;"></span>
                ${carName}
            </div>
            <div style="font-size:0.75rem;color:#64748b;margin-bottom:3px;">Coordinates: <b style="color:#0f172a;font-family:monospace;">${lat.toFixed(5)}, ${lng.toFixed(5)}</b></div>
            <div style="font-size:0.72rem;color:#64748b;margin-bottom:8px;">Satellites: <b style="color:#007bff;">${sats}</b></div>
            <button onclick="window.open('https://www.google.com/maps?q=${lat},${lng}','_blank')" style="width:100%;background:#007bff;color:white;border:none;border-radius:6px;padding:6px 10px;font-size:0.75rem;font-weight:700;cursor:pointer;">
                Google Maps Navigate
            </button>
        </div>
    `;
    marker.bindPopup(content);
}

const carIcon = createYiPinIcon('car1', '01·CAR-001', false);
let carMarker = L.marker([14.2600, 121.3958], { icon: carIcon });
let markerIsOnMap = false;

const carIcon2 = createYiPinIcon('car2', '02·CAR-002', true);
let carMarker2 = L.marker([14.2600, 121.3958], { icon: carIcon2 });
let markerIsOnMap2 = false;

// State Variables
let lastActiveTime = Date.now();
let isDecrypted = false; // Default: Encrypted/Locked

let lastActiveTime2 = Date.now();
let isDecrypted2 = false; // Default: Encrypted/Locked

// --- FLEET BAR & VIEW ALL DEVICES LOGIC ---
function updateFleetBar() {
    const textEl = document.getElementById("fleet-count-text");
    const count = (markerIsOnMap ? 1 : 0) + (markerIsOnMap2 ? 1 : 0);
    if (textEl) {
        if (count === 0) {
            textEl.textContent = "Fleet Radar: No Live Pins";
        } else if (count === 1) {
            textEl.textContent = "Fleet Radar: 1 Vehicle Live";
        } else {
            textEl.textContent = `Fleet Radar: ${count} Vehicles Live`;
        }
    }
}

function fitAllDevices() {
    const points = [];
    if (markerIsOnMap && carMarker) {
        points.push(carMarker.getLatLng());
    }
    if (markerIsOnMap2 && carMarker2) {
        points.push(carMarker2.getLatLng());
    }

    if (points.length === 0) {
        alert("No decrypted active devices currently on map. Click the car icon to decrypt location first.");
    } else if (points.length === 1) {
        map.setView(points[0], 16);
    } else {
        map.fitBounds(points, { padding: [80, 80], maxZoom: 16 });
    }
}

// --- UI EVENT LISTENERS ---
document.getElementById("car-trigger").addEventListener("click", () => {
    const panel = document.getElementById("details-panel");
    const willOpen = panel.style.display !== "block";
    panel.style.display = willOpen ? "block" : "none";
    if (willOpen) document.getElementById("details-panel-2").style.display = "none";
});

document.getElementById("locate-btn").addEventListener("click", () => {
    if (markerIsOnMap) {
        map.setView(carMarker.getLatLng(), 16);
    } else {
        alert("Location is currently encrypted or offline.");
    }
});

document.getElementById("car-trigger-2").addEventListener("click", () => {
    const panel = document.getElementById("details-panel-2");
    const willOpen = panel.style.display !== "block";
    panel.style.display = willOpen ? "block" : "none";
    if (willOpen) document.getElementById("details-panel").style.display = "none";
});

document.getElementById("locate-btn-2").addEventListener("click", () => {
    if (markerIsOnMap2) {
        map.setView(carMarker2.getLatLng(), 16);
    } else {
        alert("Location is currently encrypted or offline.");
    }
});

// --- DECRYPT BUTTON LOGIC ---
function toggleOverride() {
    isDecrypted = !isDecrypted;
    const btn = document.getElementById("override-btn");
    
    if (isDecrypted) {
        btn.innerText = "🔒 Re-Encrypt Location";
        btn.style.backgroundColor = "#dc3545";
        btn.style.color = "white";
    } else {
        btn.innerText = "🔓 Decrypt Car Location";
        btn.style.backgroundColor = "#ffc107";
        btn.style.color = "#212529";
    }
    updateDashboard();
}

function toggleOverride2() {
    isDecrypted2 = !isDecrypted2;
    const btn = document.getElementById("override-btn-2");

    if (isDecrypted2) {
        btn.innerText = "🔒 Re-Encrypt Location";
        btn.style.backgroundColor = "#dc3545";
        btn.style.color = "white";
    } else {
        btn.innerText = "🔓 Decrypt Car Location";
        btn.style.backgroundColor = "#ffc107";
        btn.style.color = "#212529";
    }
    updateDashboard2();
}

// --- CORE DASHBOARD LOGIC ---
async function updateDashboard() {
  try {
    const r = await fetch(`https://${blynkServer}/external/api/get?token=${blynkToken}&v1&v2&v3&v4&_=${Date.now()}`);
    const data = await r.json();
    const now = Date.now();
    
    if (data) {
      if (data.v4 !== undefined) {
        lastActiveTime = now;
      }
      const isOnline = (now - lastActiveTime < 20000);
      
      // Vanish if offline
      if (!isOnline) {
          document.getElementById("car-trigger").style.display = "none";
          document.getElementById("details-panel").style.display = "none";
          if (markerIsOnMap) { map.removeLayer(carMarker); markerIsOnMap = false; }
          updateFleetBar();
          return;
      }

      // PRIVACY/ENCRYPTION GATEKEEPER
      if (!isDecrypted) {
          // --- ENCRYPTED STATE ---
          document.getElementById("ui-lat").innerText = "*** ENCRYPTED ***";
          document.getElementById("ui-lng").innerText = "*** ENCRYPTED ***";
          document.getElementById("ui-sats").innerText = "Hidden";
          document.getElementById("ui-privacy-badge").innerText = "Security: Encrypted";
          document.getElementById("ui-privacy-badge").style.background = "#fff3cd"; 
          document.getElementById("ui-privacy-badge").style.color = "#856404";
          
          if (markerIsOnMap) { map.removeLayer(carMarker); markerIsOnMap = false; }
          updateFleetBar();
      } else {
          // --- DECRYPTED STATE ---
          updateCoordinates(data);
          document.getElementById("ui-privacy-badge").innerText = "Security: Decrypted";
          document.getElementById("ui-privacy-badge").style.background = "#d4edda";
          document.getElementById("ui-privacy-badge").style.color = "#155724";
      }

      // UI Cleanup
      document.getElementById("car-trigger").style.display = "block";
      document.getElementById("ui-dot").style.backgroundColor = "#28a745";
    }
  } catch (e) { console.log("Fetch error"); }
}

function updateCoordinates(data) {
    document.getElementById("ui-lat").innerText = data.v1 || "--";
    document.getElementById("ui-lng").innerText = data.v2 || "--";
    document.getElementById("ui-sats").innerText = data.v3 || "0";
    const lat = parseFloat(data.v1);
    const lng = parseFloat(data.v2);
    if (!isNaN(lat) && !isNaN(lng)) {
        carMarker.setLatLng([lat, lng]);
        updateMarkerPopup(carMarker, 'Car-001 (01·CAR-001)', lat, lng, data.v3 || '0');
        if (!markerIsOnMap) { carMarker.addTo(map); markerIsOnMap = true; }
        updateFleetBar();
    }
}

async function updateDashboard2() {
  try {
    const r = await fetch(`https://${blynkServer}/external/api/get?token=${blynkToken}&v5&v6&v7&v8&_=${Date.now()}`);
    const data = await r.json();
    const now = Date.now();

    if (data) {
      if (data.v8 !== undefined) {
        lastActiveTime2 = now;
      }
      const isOnline = (now - lastActiveTime2 < 20000);

      if (!isOnline) {
          document.getElementById("car-trigger-2").style.display = "none";
          document.getElementById("details-panel-2").style.display = "none";
          if (markerIsOnMap2) { map.removeLayer(carMarker2); markerIsOnMap2 = false; }
          updateFleetBar();
          return;
      }

      if (!isDecrypted2) {
          document.getElementById("ui-lat-2").innerText = "*** ENCRYPTED ***";
          document.getElementById("ui-lng-2").innerText = "*** ENCRYPTED ***";
          document.getElementById("ui-sats-2").innerText = "Hidden";
          document.getElementById("ui-privacy-badge-2").innerText = "Security: Encrypted";
          document.getElementById("ui-privacy-badge-2").style.background = "#fff3cd";
          document.getElementById("ui-privacy-badge-2").style.color = "#856404";

          if (markerIsOnMap2) { map.removeLayer(carMarker2); markerIsOnMap2 = false; }
          updateFleetBar();
      } else {
          updateCoordinates2(data);
          document.getElementById("ui-privacy-badge-2").innerText = "Security: Decrypted";
          document.getElementById("ui-privacy-badge-2").style.background = "#d4edda";
          document.getElementById("ui-privacy-badge-2").style.color = "#155724";
      }

      document.getElementById("car-trigger-2").style.display = "block";
      document.getElementById("ui-dot-2").style.backgroundColor = "#28a745";
    }
  } catch (e) { console.log("Fetch error (Car-002)"); }
}

function updateCoordinates2(data) {
    document.getElementById("ui-lat-2").innerText = data.v5 || "--";
    document.getElementById("ui-lng-2").innerText = data.v6 || "--";
    document.getElementById("ui-sats-2").innerText = data.v7 || "0";
    const lat = parseFloat(data.v5);
    const lng = parseFloat(data.v6);
    if (!isNaN(lat) && !isNaN(lng)) {
        carMarker2.setLatLng([lat, lng]);
        updateMarkerPopup(carMarker2, 'Car-002 (02·CAR-002)', lat, lng, data.v7 || '0');
        if (!markerIsOnMap2) { carMarker2.addTo(map); markerIsOnMap2 = true; }
        updateFleetBar();
    }
}

// Initial run and auto-refresh every 5 seconds
updateDashboard();
setInterval(updateDashboard, 5000);

updateDashboard2();
setInterval(updateDashboard2, 5000);