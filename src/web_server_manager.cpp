#include "web_server_manager.h"
#include "config.h"
#include "display.h"
#include "wifi_manager.h"
#include "time_manager.h"
#include "settings_manager.h"
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClient.h>
#include "version.h"
#include "weather_manager.h"
#include <ArduinoJson.h>

WebServerManager webServer;

WebServerManager::WebServerManager() : server(WEB_SERVER_PORT) {}

void WebServerManager::begin() {
    server.on("/", [this]() { handleRoot(); });
    server.on("/api/settings", HTTP_GET, [this]() { handleGetSettings(); });
    server.on("/api/settings", HTTP_POST, [this]() { handleSaveSettings(); });
    server.on("/api/restart", HTTP_POST, [this]() { handleRestart(); });
    server.on("/api/reset", HTTP_POST, [this]() { handleReset(); });
    server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/api/update", HTTP_POST, [this]() { handleOTAUpdate(); });
    server.onNotFound([this]() { handleNotFound(); });
    
    server.begin();
    Serial.println("Webserver gestartet auf Port " + String(WEB_SERVER_PORT));
}

void WebServerManager::handleClient() {
    server.handleClient();
}

String WebServerManager::getHTML() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>OBEGRÄNSAD-X Konfiguration</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            max-width: 500px;
            width: 100%;
            padding: 40px;
        }
        h1 {
            color: #333;
            margin-bottom: 10px;
            font-size: 28px;
        }
        .subtitle {
            color: #666;
            margin-bottom: 30px;
            font-size: 14px;
        }
        .info-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin-bottom: 25px;
        }
        .info-card {
            background: #f0f4ff;
            border-left: 4px solid #667eea;
            padding: 15px;
            border-radius: 8px;
        }
        .info-card strong {
            color: #667eea;
            display: block;
            margin-bottom: 5px;
            font-size: 12px;
        }
        .info-card span {
            color: #333;
            font-size: 16px;
            font-weight: 600;
        }
        .live-indicator {
            display: inline-block;
            width: 8px;
            height: 8px;
            background: #28a745;
            border-radius: 50%;
            margin-right: 5px;
            animation: pulse 2s infinite;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        .form-group {
            margin-bottom: 25px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 600;
            font-size: 14px;
        }
        input[type="range"] {
            width: 100%;
            height: 8px;
            border-radius: 5px;
            background: #ddd;
            outline: none;
            -webkit-appearance: none;
        }
        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #667eea;
            cursor: pointer;
        }
        input[type="range"]::-moz-range-thumb {
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #667eea;
            cursor: pointer;
            border: none;
        }
        .value-display {
            text-align: center;
            font-size: 24px;
            font-weight: bold;
            color: #667eea;
            margin-top: 10px;
        }
        select, input[type="text"] {
            width: 100%;
            padding: 12px;
            border: 2px solid #ddd;
            border-radius: 8px;
            font-size: 16px;
            background: white;
            cursor: pointer;
        }
        select:focus, input[type="text"]:focus {
            outline: none;
            border-color: #667eea;
        }
        .button-group {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            margin-top: 30px;
        }
        button {
            padding: 15px;
            border: none;
            border-radius: 8px;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s;
        }
        .btn-primary {
            background: #667eea;
            color: white;
        }
        .btn-primary:hover {
            background: #5568d3;
            transform: translateY(-2px);
        }
        .btn-secondary {
            background: #e0e0e0;
            color: #333;
        }
        .btn-secondary:hover {
            background: #d0d0d0;
        }
        .btn-danger {
            background: #dc3545;
            color: white;
        }
        .btn-danger:hover {
            background: #c82333;
        }
        .status {
            margin-top: 20px;
            padding: 15px;
            border-radius: 8px;
            text-align: center;
            font-weight: 600;
            display: none;
        }
        .status.success {
            background: #d4edda;
            color: #155724;
            display: block;
        }
        .status.error {
            background: #f8d7da;
            color: #721c24;
            display: block;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>OBEGRÄNSAD-X</h1>
        <p class="subtitle">Konfiguration</p>
        
        <div class="info-grid">
            <div class="info-card">
                <strong>WLAN</strong>
                <span id="ssid">)rawliteral" + wifiConnection.getSSID() + R"rawliteral(</span>
            </div>
            <div class="info-card">
                <strong>IP-Adresse</strong>
                <span id="ip">)rawliteral" + wifiConnection.getIP() + R"rawliteral(</span>
            </div>
            <div class="info-card">
                <strong>Signal</strong>
                <span id="rssi">)rawliteral" + String(wifiConnection.getRSSI()) + R"rawliteral( dBm</span>
            </div>
            <div class="info-card">
                <strong><span class="live-indicator"></span>Zeit</strong>
                <span id="time">)rawliteral" + timeManager.getTimeString() + R"rawliteral(</span>
            </div>
        </div>
        
        <form id="settingsForm">
            <div class="form-group">
                <label for="brightness">Helligkeit</label>
                <input type="range" id="brightness" name="brightness" min="10" max="255" value=")rawliteral" + String(settingsManager.getBrightness()) + R"rawliteral(" oninput="updateValue(this.value)">
                <div class="value-display" id="brightnessValue">)rawliteral" + String(settingsManager.getBrightness()) + R"rawliteral(</div>
            </div>
            
            <div class="form-group">
                <label for="mode">Anzeigemodus</label>
                <select id="mode" name="mode">
                    <option value="0" )rawliteral" + String(settingsManager.getDisplayMode() == 0 ? "selected" : "") + R"rawliteral(>Uhrzeit (HH:MM)</option>
                    <option value="1" )rawliteral" + String(settingsManager.getDisplayMode() == 1 ? "selected" : "") + R"rawliteral(>Sekunden</option>
                    <option value="2" )rawliteral" + String(settingsManager.getDisplayMode() == 2 ? "selected" : "") + R"rawliteral(>Datum (TT.MM)</option>
                    <option value="3" )rawliteral" + String(settingsManager.getDisplayMode() == 3 ? "selected" : "") + R"rawliteral(>Wetter</option>
                    <option value="4" )rawliteral" + String(settingsManager.getDisplayMode() == 4 ? "selected" : "") + R"rawliteral(>Automatikmodus Uhrzeit/Sekunden</option>
                    <option value="5" )rawliteral" + String(settingsManager.getDisplayMode() == 5 ? "selected" : "") + R"rawliteral(>Game of Life</option>
                    <option value="6" )rawliteral" + String(settingsManager.getDisplayMode() == 6 ? "selected" : "") + R"rawliteral(>Pong</option>
                    <option value="7" )rawliteral" + String(settingsManager.getDisplayMode() == 7 ? "selected" : "") + R"rawliteral(>WiFi Signal (WIP)</option>
                    <option value="8" )rawliteral" + String(settingsManager.getDisplayMode() == 8 ? "selected" : "") + R"rawliteral(>Matrix Rain (WIP)</option>
                    <option value="9" )rawliteral" + String(settingsManager.getDisplayMode() == 9 ? "selected" : "") + R"rawliteral(>Display aus</option>
                </select>
            </div>

            <div class="form-group">
                <label for="city">Wetter · Stadt</label>
                <input type="text" id="city" name="city" placeholder="z. B. Berlin" value=")rawliteral" + settingsManager.getCity() + R"rawliteral(" />
            </div>
            
            <div class="button-group">
                <button type="submit" class="btn-primary">Speichern</button>
                <button type="button" class="btn-secondary" onclick="restart()">Neustart</button>
                <button type="button" class="btn-secondary" onclick="firmwareUpdate()">Firmware-Update</button>
                <button type="button" class="btn-danger" onclick="resetWiFi()">WiFi Reset</button>
            </div>
        </form>
        
        <div id="status" class="status"></div>
    </div>
    
    <script>
        setInterval(async () => {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();
                document.getElementById('time').textContent = data.time;
                document.getElementById('rssi').textContent = data.rssi + ' dBm';
            } catch (error) {
                console.error('Status update failed:', error);
            }
        }, 1000);
        
        function updateValue(val) {
            document.getElementById('brightnessValue').textContent = val;
        }
        
        document.getElementById('settingsForm').addEventListener('submit', async (e) => {
            e.preventDefault();
            const data = {
                brightness: parseInt(document.getElementById('brightness').value),
                mode: parseInt(document.getElementById('mode').value),
                city: (document.getElementById('city').value || '').trim()
            };
            
            try {
                const response = await fetch('/api/settings', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(data)
                });
                if (response.ok) {
                    showStatus('Einstellungen gespeichert!', 'success');
                } else {
                    showStatus('Fehler beim Speichern!', 'error');
                }
            } catch (error) {
                showStatus('Fehler beim Speichern!', 'error');
            }
        });
        
        async function restart() {
            if (confirm('Gerät neu starten?')) {
                await fetch('/api/restart', { method: 'POST' });
                showStatus('Neustart...', 'success');
            }
        }
        
        async function resetWiFi() {
            if (confirm('WiFi-Einstellungen zurücksetzen? Das Gerät startet neu.')) {
                await fetch('/api/reset', { method: 'POST' });
                showStatus('WiFi wird zurückgesetzt...', 'success');
            }
        }

        async function firmwareUpdate() {
            if (confirm('Firmware-Update aus GitHub starten?')) {
                showStatus('Starte Update...', 'success');
                try {
                    await fetch('/api/update', { method: 'POST' });
                    showStatus('Update läuft, Gerät startet neu...', 'success');
                } catch (e) {
                    showStatus('Update fehlgeschlagen!', 'error');
                }
            }
        }
        
        function showStatus(msg, type) {
            const status = document.getElementById('status');
            status.className = 'status ' + type;
            status.textContent = msg;
            setTimeout(() => status.style.display = 'none', 3000);
        }
    </script>
</body>
</html>
)rawliteral";
    
    return html;
}

void WebServerManager::handleRoot() {
    server.send(200, "text/html", getHTML());
}

void WebServerManager::handleGetSettings() {
    String json = "{";
    json += "\"brightness\":" + String(settingsManager.getBrightness()) + ",";
    json += "\"mode\":" + String(settingsManager.getDisplayMode()) + ",";
    json += "\"city\":\"" + settingsManager.getCity() + "\"";
    json += "}";
    
    server.send(200, "application/json", json);
}

void WebServerManager::handleSaveSettings() {
    if (server.hasArg("plain")) {
        String body = server.arg("plain");
        
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"invalid json\"}");
            return;
        }

        if (doc["brightness"].is<int>()) {
            int value = doc["brightness"].as<int>();
            settingsManager.setBrightness(value);
            display.setBrightness(value);
        }
        if (doc["mode"].is<int>()) {
            int value = doc["mode"].as<int>();
            settingsManager.setDisplayMode(value);
        }
        if (doc["city"].is<const char*>()) {
            String value = String(doc["city"].as<const char*>());
            if (value.length() > 0) {
                settingsManager.setCity(value);
                weatherManager.setCity(settingsManager.getCity());
                // Sofort aktualisieren (ohne Haken-Animation)
                weatherManager.update(true);
            }
        }

        server.send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        server.send(400, "application/json", "{\"status\":\"error\"}");
    }
}

void WebServerManager::handleStatus() {
    String json = "{";
    json += "\"ssid\":\"" + wifiConnection.getSSID() + "\",";
    json += "\"ip\":\"" + wifiConnection.getIP() + "\",";
    json += "\"rssi\":" + String(wifiConnection.getRSSI()) + ",";
    json += "\"time\":\"" + timeManager.getTimeString() + "\",";
    json += "\"date\":\"" + timeManager.getDateString() + "\",";
    json += "\"uptime\":" + String(millis() / 1000);
    json += "}";
    
    server.send(200, "application/json", json);
}

void WebServerManager::handleRestart() {
    server.send(200, "text/plain", "Restarting...");
    delay(1000);
    ESP.restart();
}

void WebServerManager::handleReset() {
    server.send(200, "text/plain", "Resetting WiFi...");
    delay(1000);
    wifiConnection.reset();
}

void WebServerManager::handleNotFound() {
    if (!wifiConnection.isConnected()) {
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    } else {
        server.send(404, "text/plain", "404: Seite nicht gefunden");
    }
}

void WebServerManager::handleOTAUpdate() {
    Serial.println("[OTA] Firmware-Update angefordert (Web API)");
    server.send(200, "text/plain", "Starte Firmware-Update...");

    display.clear();
    display.drawText2x2("UPDT");
    display.update();

    // Versionscheck
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(OTA_VERSION_URL);
    int code = http.GET();

    if (code != 200) {
        Serial.printf("[OTA] Fehler beim Abruf der Version (%d)\n", code);
        display.clear();
        display.drawText2x2("ERR");
        display.update();
        delay(1500);
        http.end();
        return;
    }

    String newVersion = http.getString();
    newVersion.trim();
    Serial.printf("[OTA] Online-Version: %s | Lokal: %s\n", newVersion.c_str(), CURRENT_VERSION);
    http.end();

    if (newVersion == CURRENT_VERSION) {
        Serial.println("[OTA] Firmware ist aktuell.");
        display.clear();
        display.drawText2x2("OK");
        display.update();
        delay(1500);
        return;
    }

    Serial.println("[OTA] Neue Version verfügbar! Starte Update...");

    // OTA mit HTTPClient + Update.writeStream()
    WiFiClientSecure client;
    client.setInsecure();  // HTTPS akzeptiert alle Zertifikate

    HTTPClient httpUpdate;
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    httpUpdate.begin(client, OTA_FIRMWARE_URL);
    int httpCode = httpUpdate.GET();

    if (httpCode != 200) {
        Serial.printf("[OTA] Fehler beim Firmware-Download (%d)\n", httpCode);
        display.clear();
        display.drawText2x2("ERR");
        display.update();
        delay(1500);
        httpUpdate.end();
        return;
    }

    int len = httpUpdate.getSize();
    if (!Update.begin(len)) {
        Serial.println("[OTA] Nicht genug Speicher für Update!");
        display.clear();
        display.drawText2x2("ERR");
        display.update();
        delay(1500);
        httpUpdate.end();
        return;
    }

    WiFiClient *stream = httpUpdate.getStreamPtr();
    size_t written = Update.writeStream(*stream);

    if (Update.end() && Update.isFinished()) {
        Serial.printf("[OTA] Update erfolgreich (%d Bytes)\n", written);
        display.clear();
        display.drawText2x2("DONE");
        display.update();
        delay(1500);
        ESP.restart();
    } else {
        Serial.println("[OTA] Fehler beim Schreiben der Firmware!");
        display.clear();
        display.drawText2x2("ERR");
        display.update();
        delay(1500);
    }

    httpUpdate.end();
}
