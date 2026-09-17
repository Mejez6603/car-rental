# 🚗 Live Fleet Tracker — Car-001 & Car-002 (Secure Edition)

A professional-grade, privacy-conscious vehicle tracking system. This system allows the driver to toggle "Privacy Mode" physically from the car, while providing the owner with "Master Override" capabilities via a secure web dashboard.

![](image/image001.png) 

---

## 📝 Description
This project provides a real-time, high-security vehicle monitoring solution. It features a unique **"Default Secure"** architecture: tracking data is encrypted by default on the web dashboard to protect driver privacy. Authorized users can decrypt the location on-demand via a Master Override, and drivers can toggle visibility instantly using a hardware privacy button inside the vehicle.

The dashboard now tracks a small **fleet of two vehicles** (Car-001 and Car-002), each with its own floating bubble and detail panel on the same map, sharing one Blynk device but kept separate on the wire via distinct virtual pins.

## ✨ Key Security Features
* **Default Secure:** All location data is encrypted (`*** ENCRYPTED ***`) on the dashboard by default.
* **Hardware Privacy Toggle:** A physical button allows the driver to toggle Privacy Mode on/off, providing instant, tactile control.
* **Master Override:** Owners can reveal the location of an "Encrypted" vehicle by clicking the **"🔓 Decrypt Car Location"** button on the dashboard.
* **Automatic Vanish:** To prevent stale data, the system automatically removes the car marker from the map if it goes offline for more than 20 seconds.
* **Non-Blocking Logic:** The hardware handles privacy transitions instantly without needing system reboots, ensuring continuous connectivity.

## ⚙️ Updated Architecture
1. **Data Acquisition:** Each vehicle's ESP32 collects GPS data and monitors its own privacy control.
2. **Transmission:** Data is batched and sent to the Blynk Cloud. Car-001 and Car-002 currently share a single Blynk device/token, told apart purely by virtual pin number:
    - **Car-001:** `v1` = lat, `v2` = lng, `v3` = sats, `v4` = heartbeat
    - **Car-002:** `v5` = lat, `v6` = lng, `v7` = sats, `v8` = heartbeat
3. **Encryption Gatekeeper:** The dashboard fetches telemetry per car and applies security layers:
    - If `Privacy Mode == ON` AND `Override == OFF`: Marker hidden, data encrypted.
    - If `Privacy Mode == OFF` OR `Override == ON`: Marker visible, data decrypted.
4. **Offline Watchdog:** The dashboard calculates a 20-second heartbeat window per car to determine device connectivity.
5. **Car-002's own local dashboard:** Car-002's ESP32 (LilyGO A7670E + Neo-6M GPS) also hosts its own small captive-portal web page over its local Wi-Fi AP, independent of the main fleet dashboard. From there you can view live GPS/network status, toggle its OLED privacy screen, and edit its Blynk server/port/auth token (**Cloud Sync**) without reflashing.

## 🚀 Recent Changelog

### v3.0.0 (Two-Car Fleet & Reliability Update)
* **Added:** Car-002 to the fleet dashboard — its own bubble and detail panel, same "Default Secure" concept as Car-001, stacked directly below it and color-coded (blue) so the two are easy to tell apart at a glance.
* **Added:** Cloud Sync panel on Car-002's local Wi-Fi portal, so its Blynk server/port/auth token can be changed without reflashing firmware.
* **Added:** Automatic cellular reconnect on Car-002's firmware — a dropped or failed connection now retries every 30s instead of staying down until the device is power-cycled.
* **Added:** Stale-GPS detection on Car-002's firmware — a fix that stops updating (lost antenna, dead module) now correctly falls back to "Searching..." instead of reporting a frozen position as live.
* **Fixed:** Car-002's cellular modem never powering on — its ESP32 firmware was missing the required `BOARD_POWERON_PIN` (GPIO 12) that gates power to the A7670E chip, and was leaving `MODEM_RESET` permanently asserted after boot.
* **Fixed:** A firmware compile error (`Please define GSM modem model`) caused by targeting a modem macro (`TINY_GSM_MODEM_A7670`) that only exists in LilyGO's own unlisted TinyGSM fork, not the version installed via Arduino Library Manager.
* **Fixed:** Car-002's dashboard bubble rendering with no visible styling — `style.css` had written the badge/button rules as ID-only selectors tied to Car-001's exact element ids, so they silently never applied to Car-002. Converted to shared classes.
* **Fixed:** The two cars' floating panels overlapping on screen when both had data open at once. The panels are now mutually exclusive (opening one closes the other) and the bubbles are cleanly stacked instead of relying on guessed pixel spacing.
* **Fixed:** Map tiles rendering with a large "API KEY REQUIRED" watermark after CARTO's basemap policy change (Aug 2026) started requiring a key for anonymous tile requests. Added a CARTO API key to the tile URL.
* **Fixed:** A dead reference to a non-existent `#ui-time` element in `script.js` that silently threw on every successful poll and got logged as a misleading "Fetch error".

### v2.0.0 (Privacy-Focused Update)
* **Added:** Hardware Privacy Mode button with OLED feedback.
* **Added:** "Default Secure" encryption layer on the web dashboard.
* **Added:** Master Override feature for authorized fleet owners.
* **Improved:** Offline detection reduced from 90s to 20s for tighter security.
* **Fixed:** Resolved "static snow" on OLED during startup with a 1000ms delay.
* **Improved:** Modular codebase (split into `index.html`, `style.css`, and `script.js`).

## 🐛 Troubleshooting Log / Lessons Learned
Notes from getting Car-002 online, kept here so the same issues aren't re-debugged from scratch next time:

* **Modem's red LED never lit, no signal ever found.** Root cause was a missing power-enable step: LilyGO's A7670E boards gate the modem's own power rail behind a dedicated GPIO (12) that our sketch never drove high, so the chip was never actually powered — separately from PWRKEY/RESET. Confirmed against LilyGO's own official example code and fixed by adding that pin plus properly releasing `MODEM_RESET` after boot (it was being left asserted forever).
* **Compile error after "fixing" the modem type define.** Switching to `TINY_GSM_MODEM_A7670` (to match the chip's real name) broke the build with `#error "Please define GSM modem model"`. The mainline TinyGSM library — the one Arduino's Library Manager installs — has no A7670 macro at all; only LilyGO's own fork does, and it isn't indexed in Library Manager. Reverted to `TINY_GSM_MODEM_SIM7600`, which the A76xx chip is command-compatible with.
* **Car-002 showed up with broken/unstyled buttons.** Turned out `style.css` styled the privacy badge and buttons by ID (`#override-btn`, etc.), which only ever matched Car-001's elements. Fixed by moving that styling onto shared classes so any car added to the dashboard gets the same look automatically.
* **The two car bubbles/panels overlapped each other.** An early fix stacked them with fixed pixel offsets, which broke as soon as a panel's real height (or both panels open at once) exceeded what was assumed. Solved by making the two panels mutually exclusive instead of trying to hand-tune more magic numbers.
* **Map suddenly covered in an "API KEY REQUIRED" watermark.** Not a bug in this project — CARTO changed their basemap tile policy around August 2026 and started watermarking anonymous (keyless) tile requests. Fixed with a free CARTO API key appended to the tile URL.
* **Confusing virtual pin mismatch between firmware and dashboard.** The dashboard read `v1-v4` while Car-002's firmware posted to `v5-v8`; this looked like a bug until it was confirmed both cars intentionally share one Blynk token/device and are told apart purely by virtual pin number rather than separate devices.

## 🛠️ Updated Technologies
* **Hardware (Car-001):** ESP32, SIM800L/Air780E, GPS, SSD1306 OLED, Toggle Button.
* **Hardware (Car-002):** ESP32 (LilyGO T-A7670E), Neo-6M GPS, SSD1306 OLED, local Wi-Fi captive portal (no physical privacy button — toggled from its own local dashboard).
* **Frontend:** HTML5, CSS3, JavaScript (Leaflet.js for Mapping).
* **Backend:** Blynk IoT REST API (Secure Batch Updates), one shared device across both cars.

## 🚀 Getting Started
1. **Blynk:** Ensure `v1-v4` (Car-001) and `v5-v8` (Car-002) are set up on the same device/token, and your dashboard logic handles the encryption flow for both.
2. **Wiring (Car-001):** Ensure the Privacy Button is connected to GPIO 4 (Input Pullup).
3. **Wiring (Car-002):** A7670E `BOARD_POWERON_PIN` must be wired/enabled on GPIO 12 in addition to PWRKEY (GPIO 4) and RESET (GPIO 5) — see the firmware's power-on sequence.
4. **Setup:** Open `script.js` and set your `blynkToken`, `blynkServer`, and CARTO `cartoApiKey` (free key from [carto.com/basemaps/apikey](https://carto.com/basemaps/apikey)).
5. **Car-002 Cloud Sync:** Alternatively, set Car-002's Blynk server/port/auth token from its own local Wi-Fi portal instead of hardcoding them in firmware.
6. **Deploy:** Upload the appropriate `.ino` sketch to each car's ESP32.

## ⏩ Future Enhancements
* **Trip History:** Storing historical coordinate arrays for automated route playback.
* **Geofencing:** Automated alerts if the vehicle leaves the Santa Cruz/Laguna operating zone.
* **Battery Monitoring:** Voltage telemetry to warn the owner if the car battery is draining while parked.
* **Separate Blynk devices per car:** Move off the shared-token/virtual-pin-offset scheme as the fleet grows past a couple of vehicles.

## 🤝 Acknowledgements
* **Leaflet.js** for the mapping library.
* **Blynk IoT** for the seamless cloud integration.
* **CARTO** for the map tile styles.

## 📄 License
This project is licensed under the MIT License - see the LICENSE file for details.
