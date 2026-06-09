const blynkToken = "6fub_AeSZfywBab9j-d7KRXWKFPMwIxz";
const blynkServer = "sgp1.blynk.cloud"; 

const map = L.map('map').setView([14.2600, 121.3958], 16);
L.tileLayer('https://{s}.basemaps.cartocdn.com/rastertiles/voyager/{z}/{x}/{y}{r}.png').addTo(map);

const carIcon = L.icon({
    iconUrl: 'https://cdn-icons-png.flaticon.com/512/744/744465.png',
    iconSize: [40, 40], iconAnchor: [20, 20]
});
let carMarker = L.marker([14.2600, 121.3958], {icon: carIcon});
let markerIsOnMap = false; 

// State Variables
let lastActiveTime = Date.now();
let lastV4Value = null;
let isDecrypted = false; // Default: Encrypted/Locked

// --- UI EVENT LISTENERS ---
document.getElementById("car-trigger").addEventListener("click", () => {
    const panel = document.getElementById("details-panel");
    panel.style.display = (panel.style.display === "block") ? "none" : "block";
});

document.getElementById("locate-btn").addEventListener("click", () => {
    if (markerIsOnMap) {
        map.setView(carMarker.getLatLng(), 16);
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

// --- CORE DASHBOARD LOGIC ---
async function updateDashboard() {
  try {
    const r = await fetch(`https://${blynkServer}/external/api/get?token=${blynkToken}&v1&v2&v3&v4&_=${Date.now()}`);
    const data = await r.json();
    const now = Date.now();
    
    if (data) {
      // 1. Connection Status (20-second timeout)
      if (data.v4 !== undefined && data.v4 !== lastV4Value) {
        lastActiveTime = now;
        lastV4Value = data.v4;
      }
      const isOnline = (now - lastActiveTime < 20000);
      
      // Vanish if offline
      if (!isOnline) {
          document.getElementById("car-trigger").style.display = "none";
          document.getElementById("details-panel").style.display = "none";
          if (markerIsOnMap) { map.removeLayer(carMarker); markerIsOnMap = false; }
          return;
      }

      // 2. PRIVACY/ENCRYPTION GATEKEEPER
      if (!isDecrypted) {
          // --- ENCRYPTED STATE ---
          document.getElementById("ui-lat").innerText = "*** ENCRYPTED ***";
          document.getElementById("ui-lng").innerText = "*** ENCRYPTED ***";
          document.getElementById("ui-sats").innerText = "Hidden";
          document.getElementById("ui-privacy-badge").innerText = "Security: Encrypted";
          document.getElementById("ui-privacy-badge").style.background = "#fff3cd"; 
          document.getElementById("ui-privacy-badge").style.color = "#856404";
          
          if (markerIsOnMap) { map.removeLayer(carMarker); markerIsOnMap = false; }
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
      document.getElementById("ui-time").innerText = new Date().toLocaleTimeString();
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
        if (!markerIsOnMap) { carMarker.addTo(map); markerIsOnMap = true; }
    }
}

// Initial run and auto-refresh every 5 seconds
updateDashboard();
setInterval(updateDashboard, 5000);