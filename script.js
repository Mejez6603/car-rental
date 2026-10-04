const blynkToken = "6fub_AeSZfywBab9j-d7KRXWKFPMwIxz";
const blynkServer = "sgp1.blynk.cloud";
// Car-002 shares the same Blynk token/device as Car-001, distinguished by
// virtual pins instead: Car-001 = v1-v4, Car-002 = v5-v8.

const cartoApiKey = "cb1_3p3v_1_c7e75ca303dcbd4b49651900";

const map = L.map('map').setView([14.0893, 121.3138], 15);
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
let carMarker = L.marker([14.0893, 121.3138], { icon: carIcon });
let markerIsOnMap = false;

const carIcon2 = createYiPinIcon('car2', '02·CAR-002', true);
let carMarker2 = L.marker([14.0893, 121.3138], { icon: carIcon2 });
let markerIsOnMap2 = false;

// --- PRIVACY & DECRYPTION STATES ---
// Default: DECRYPTED AGAD pag bukas ng Admin!
let isDecrypted = true; 
let isDecrypted2 = true;

// Track if renter explicitly requested privacy opt-out
let privacyRequested1 = false;
let privacyRequested2 = false;
let renterName1 = '';
let renterName2 = '';

let lastActiveTime = Date.now();
let lastActiveTime2 = Date.now();

// --- CHECK RENTER PRIVACY OPT-OUT FROM AUTORIDE BACKEND ---
async function checkRenterPrivacyOptOut() {
    try {
        const res = await fetch('https://autoride-booking-system.vercel.app/api/gps-privacy-status');
        if (res.ok) {
            const data = await res.json();
            // Data contains vehicles whose current renter opted out of GPS tracking
            for (const [vId, info] of Object.entries(data || {})) {
                const combined = `${info.brand || ''} ${info.model || ''} ${info.plate_number || ''} ${info.gps_device_name || ''}`.toUpperCase();
                
                // If it matches Montero Sport, Vehicle #12, or Car-001
                if (combined.includes('MONTERO') || combined.includes('DUY 6527') || combined.includes('CAR-001') || vId === '12') {
                    privacyRequested1 = true;
                    renterName1 = info.customer_name || 'Renter';
                    isDecrypted = false; // Masked upon opening for this vehicle only!
                }

                // If it matches Car-002
                if (combined.includes('CAR-002') || combined.includes('VIOS') || combined.includes('UNIT 2')) {
                    privacyRequested2 = true;
                    renterName2 = info.customer_name || 'Renter';
                    isDecrypted2 = false; // Masked upon opening for this vehicle only!
                }
            }
        }
    } catch (e) {
        console.log('[GPS] Privacy check error:', e);
    }
    updateDashboard();
    updateDashboard2();
}

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
        alert("No decrypted active devices currently on map. Click a car icon to decrypt / override.");
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
        alert("Location is currently encrypted or offline. Click Emergency Override to decrypt.");
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
        alert("Location is currently encrypted or offline. Click Emergency Override to decrypt.");
    }
});

// --- DECRYPT / EMERGENCY OVERRIDE LOGIC ---
function toggleOverride() {
    isDecrypted = !isDecrypted;
    const btn = document.getElementById("override-btn");
    
    if (isDecrypted) {
        btn.innerText = "🔒 Mask Location";
        btn.style.backgroundColor = "#475569";
        btn.style.color = "white";
    } else {
        btn.innerText = "🔓 Decrypt Location (Emergency Override)";
        btn.style.backgroundColor = "#ffc107";
        btn.style.color = "#212529";
    }
    updateDashboard();
}

function toggleOverride2() {
    isDecrypted2 = !isDecrypted2;
    const btn = document.getElementById("override-btn-2");

    if (isDecrypted2) {
        btn.innerText = "🔒 Mask Location";
        btn.style.backgroundColor = "#475569";
        btn.style.color = "white";
    } else {
        btn.innerText = "🔓 Decrypt Location (Emergency Override)";
        btn.style.backgroundColor = "#ffc107";
        btn.style.color = "#212529";
    }
    updateDashboard2();
}

// --- CORE DASHBOARD LOGIC (CAR-001) ---
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

      const badge = document.getElementById("ui-privacy-badge");
      const btn = document.getElementById("override-btn");

      // PRIVACY/ENCRYPTION GATEKEEPER
      if (!isDecrypted) {
          // --- ENCRYPTED STATE (e.g. Renter Opted Out or Admin Masked) ---
          document.getElementById("ui-lat").innerText = "*** ENCRYPTED ***";
          document.getElementById("ui-lng").innerText = "*** ENCRYPTED ***";
          document.getElementById("ui-sats").innerText = "Protected";
          
          if (privacyRequested1) {
              badge.innerText = `Security: Encrypted (${renterName1 || 'Renter'} Privacy Requested)`;
              badge.style.background = "#fee2e2";
              badge.style.color = "#991b1b";
              btn.innerText = "🔓 Decrypt Location (Emergency Override)";
              btn.style.backgroundColor = "#eab308";
              btn.style.color = "#000";
          } else {
              badge.innerText = "Security: Masked by Admin";
              badge.style.background = "#fff3cd";
              badge.style.color = "#856404";
              btn.innerText = "🔓 Unmask Location";
              btn.style.backgroundColor = "#ffc107";
              btn.style.color = "#212529";
          }
          
          if (markerIsOnMap) { map.removeLayer(carMarker); markerIsOnMap = false; }
          updateFleetBar();
      } else {
          // --- DECRYPTED STATE (Default Live) ---
          updateCoordinates(data);
          badge.innerText = "Security: Decrypted • Live Active";
          badge.style.background = "#d4edda";
          badge.style.color = "#155724";
          btn.innerText = "🔒 Mask Location";
          btn.style.backgroundColor = "#475569";
          btn.style.color = "white";
      }

      // UI Cleanup
      document.getElementById("car-trigger").style.display = "block";
      document.getElementById("ui-dot").style.backgroundColor = "#28a745";
    }
  } catch (e) { console.log("Fetch error (Car-001)"); }
}

function updateCoordinates(data) {
    document.getElementById("ui-lat").innerText = data.v1 || "--";
    document.getElementById("ui-lng").innerText = data.v2 || "--";
    document.getElementById("ui-sats").innerText = data.v3 || "0";
    const lat = parseFloat(data.v1);
    const lng = parseFloat(data.v2);
    if (!isNaN(lat) && !isNaN(lng)) {
        carMarker.setLatLng([lat, lng]);
        updateMarkerPopup(carMarker, 'Car-001 (Montero Sport)', lat, lng, data.v3 || '0');
        if (!markerIsOnMap) { carMarker.addTo(map); markerIsOnMap = true; }
        updateFleetBar();
    }
}

// --- CORE DASHBOARD LOGIC (CAR-002) ---
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

      const badge2 = document.getElementById("ui-privacy-badge-2");
      const btn2 = document.getElementById("override-btn-2");

      if (!isDecrypted2) {
          document.getElementById("ui-lat-2").innerText = "*** ENCRYPTED ***";
          document.getElementById("ui-lng-2").innerText = "*** ENCRYPTED ***";
          document.getElementById("ui-sats-2").innerText = "Protected";

          if (privacyRequested2) {
              badge2.innerText = `Security: Encrypted (${renterName2 || 'Renter'} Privacy Requested)`;
              badge2.style.background = "#fee2e2";
              badge2.style.color = "#991b1b";
              btn2.innerText = "🔓 Decrypt Location (Emergency Override)";
              btn2.style.backgroundColor = "#eab308";
              btn2.style.color = "#000";
          } else {
              badge2.innerText = "Security: Masked by Admin";
              badge2.style.background = "#fff3cd";
              badge2.style.color = "#856404";
              btn2.innerText = "🔓 Unmask Location";
              btn2.style.backgroundColor = "#ffc107";
              btn2.style.color = "#212529";
          }

          if (markerIsOnMap2) { map.removeLayer(carMarker2); markerIsOnMap2 = false; }
          updateFleetBar();
      } else {
          updateCoordinates2(data);
          badge2.innerText = "Security: Decrypted • Live Active";
          badge2.style.background = "#d4edda";
          badge2.style.color = "#155724";
          btn2.innerText = "🔒 Mask Location";
          btn2.style.backgroundColor = "#475569";
          btn2.style.color = "white";
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

// Initial privacy check & auto-refresh interval
checkRenterPrivacyOptOut();
setInterval(checkRenterPrivacyOptOut, 30000);

// Auto-refresh GPS telemetry every 5 seconds
updateDashboard();
setInterval(updateDashboard, 5000);

updateDashboard2();
setInterval(updateDashboard2, 5000);