
# 🚗 Car-001 Live Fleet Tracker

A real-time GPS tracking system utilizing an **ESP32** and **SIM800L** to transmit telemetric data to a web-based dashboard via the **Blynk IoT Cloud**.

## 📝 Description
This project provides a low-latency solution for vehicle monitoring. By leveraging cellular data, the system transmits coordinates, satellite strength, and a heartbeat signal to the cloud. The frontend dashboard then visualizes the car's movement on an interactive map and provides a "Watchdog" status monitoring system to notify users if the device goes offline.

## ✨ Features
* **Real-time Mapping:** Uses Leaflet.js and OpenStreetMap (Voyager tiles) for smooth vehicle tracking.
* **Watchdog Formula:** Automatically detects "Offline" status if no data is received within **90 seconds**.
* **Time Freeze Logic:** The "Last Updated" timestamp freezes when the device is offline, showing exactly when the signal was lost.
* **Batched Data:** Efficient API calls to Blynk handle Latitude, Longitude, and Satellites in a single transmission.
* **Responsive Dashboard:** Floating telemetry panel providing live GPS stats.

## ⚙️ How the System Works
1.  **Data Acquisition:** The ESP32 collects NMEA sentences from the GPS module.
2.  **Transmission:** The SIM800L sends a batched HTTP GET request to the Blynk `sgp1` server.
3.  **Cloud Storage:** Blynk stores the values in Virtual Pins (V1-V4).
4.  **Frontend Retrieval:** The HTML dashboard fetches this data every 5 seconds.
5.  **Validation:** The dashboard calculates the difference between the **current time** and the **server's `updatedAt` timestamp** to determine the connection status.

## 📜 Changelog
### v1.2.0 (Latest)
* **Fixed:** Browser refresh bug where the status would reset to "Active" regardless of actual server data.
* **Added:** V4 Heartbeat pin for more reliable connectivity monitoring.
* **Improved:** UI layout with "Voyager" map tiles for better readability.
### v1.1.0
* Implemented `parseFloat` validation to prevent "NaN" errors on the dashboard.
### v1.0.0
* Initial release with basic V1/V2/V3 tracking.

## 🛠️ Technologies Used
* **Hardware:** ESP32, SIM800L GSM Module, GPS Module.
* **Cloud/Backend:** Blynk IoT Platform (REST API).
* **Frontend:** HTML5, CSS3, JavaScript (ES6+).
* **Mapping:** Leaflet.js.

## 🚀 Getting Started
1.  **Blynk Setup:** Create four datastreams (V1: Lat, V2: Lng, V3: Sats, V4: Heartbeat).
2.  **ESP32 Config:** Upload the `.ino` sketch with your `Template ID` and `Auth Token`.
3.  **Dashboard:** Open `index.html` and replace the `blynkToken` variable with your own.

## 🌐 Deployment
The dashboard is designed for easy deployment on **Vercel** or **GitHub Pages**. Simply push the `index.html` file to your repository and connect it to your hosting provider.

## ⏩ Future Enhancements
* **Geofencing:** SMS alerts if the car leaves a specific area (Laguna area).
* **Historical Breadcrumbs:** Visualizing the path taken over the last 24 hours.
* **Power Management:** Deep sleep modes for the ESP32 to save vehicle battery when the ignition is off.

## 🤝 Acknowledgements
* **Leaflet.js** for the incredible mapping library.
* **Blynk IoT** for the seamless cloud integration.
* **CARTO** for the map tile styles.

## 📄 License
This project is licensed under the MIT License - see the LICENSE file for details.
