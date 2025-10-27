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

// 1x1 transparent GIF used for cross-origin image-based discovery
static const uint8_t PROGMEM OBEGRAENSAD_PING_GIF[] = {
    0x47,0x49,0x46,0x38,0x39,0x61,0x01,0x00,0x01,0x00,0x80,0x00,0x00,
    0x00,0x00,0x00,0xFF,0xFF,0xFF,0x21,0xF9,0x04,0x01,0x00,0x00,0x00,
    0x00,0x2C,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x02,0x02,
    0x4C,0x01,0x00,0x3B
};

WebServerManager webServer;

WebServerManager::WebServerManager() : server(WEB_SERVER_PORT) {}

void WebServerManager::begin() {
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/", HTTP_OPTIONS, [this]() { handleOptions(); });
    // fallback to legacy local UI if needed
    server.on("/local", HTTP_GET, [this]() { handleLocal(); });
    // local HTTP-only discovery UI to avoid HTTPS mixed-content
    server.on("/discover", HTTP_GET, [this]() { handleDiscover(); });
    server.on("/api/settings", HTTP_GET, [this]() { handleGetSettings(); });
    server.on("/api/settings", HTTP_POST, [this]() { handleSaveSettings(); });
    server.on("/api/settings", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/restart", HTTP_POST, [this]() { handleRestart(); });
    server.on("/api/restart", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/reset", HTTP_POST, [this]() { handleReset(); });
    server.on("/api/reset", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/api/status", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/update", HTTP_POST, [this]() { handleOTAUpdate(); });
    server.on("/api/update", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/ping.gif", HTTP_GET, [this]() { handlePing(); });
    server.on("/api/ping.gif", HTTP_OPTIONS, [this]() { handleOptions(); });
    // New neutral health endpoint to avoid ad blockers
    server.on("/api/healthz", HTTP_GET, [this]() { handleHealth(); });
    server.on("/api/healthz.gif", HTTP_GET, [this]() { handlePing(); });
    server.on("/api/healthz.png", HTTP_GET, [this]() { handlePing(); });
    server.on("/api/healthz", HTTP_OPTIONS, [this]() { handleOptions(); });
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

String WebServerManager::getDiscoverHTML() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>OBEGRÄNSAD-X – Lokaler Gerätescanner</title>
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #f3f4f6; margin: 0; }
    .wrap { max-width: 900px; margin: 0 auto; padding: 24px; }
    .card { background: #fff; border-radius: 14px; padding: 20px; box-shadow: 0 10px 30px rgba(0,0,0,.08); }
    h1 { margin: 0 0 6px; font-size: 24px; }
    p.sub { margin: 0 0 16px; color: #6b7280; font-size: 14px; }
    .row { display: flex; gap: 10px; flex-wrap: wrap; margin-bottom: 12px; }
    .select, .input { padding: 10px 12px; border: 2px solid #e5e7eb; border-radius: 10px; font-size: 15px; }
    .btn { border: 0; border-radius: 10px; padding: 10px 14px; font-weight: 600; cursor: pointer; font-size: 15px; }
    .btn-primary { background: #667eea; color: white; }
    .btn-secondary { background: #e5e7eb; color: #111827; }
    .grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(260px, 1fr)); gap: 12px; margin-top: 16px; }
    .dev { border: 1px solid #e5e7eb; border-radius: 12px; padding: 14px; background:#fff; }
    .kv { color:#6b7280; font-size: 14px; margin-top: 4px; }
    .chip { display:inline-flex; align-items:center; gap:6px; background:#eef2ff; color:#4338ca; padding:6px 8px; border-radius:999px; font-size:12px; }
    .dot { width:8px; height:8px; border-radius:50%; background:#10b981; }
    .progress { height:6px; background:#e5e7eb; border-radius:999px; overflow:hidden; }
    .bar { height:100%; width:0; background:#667eea; transition:width .2s ease; }
    .warn { color:#b45309; }
  </style>
  <script>
    // This page is served from the device over HTTP, so mixed-content restrictions do not apply.
    const gridEl = document.addEventListener ? null : null;
  </script>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <h1>OBEGRÄNSAD-X</h1>
      <p class="sub">Lokaler Gerätescanner (HTTP) – nutzbar auch von https-Seiten aus, indem Sie diesen Link öffnen.</p>
      <div class="row">
        <select id="subnetSelect" class="select">
          <option value="auto">Automatisch – 192.168.0/1, 10.0.0, 10.1.1</option>
          <option value="192.168.0">192.168.0.0/24</option>
          <option value="192.168.1">192.168.1.0/24</option>
          <option value="10.0.0">10.0.0.0/24</option>
          <option value="10.1.1">10.1.1.0/24</option>
        </select>
        <button id="scanBtn" class="btn btn-primary">Scan starten</button>
        <input id="manualIp" class="input" placeholder="IP/Hostname (z. B. 192.168.0.23 oder obegraensad-x.local)" />
        <button id="addBtn" class="btn btn-secondary">Hinzufügen</button>
      </div>
      <div class="progress"><div id="progressBar" class="bar"></div></div>
      <div id="grid" class="grid"></div>
      <p id="footerNote" class="sub"></p>
    </div>
  </div>
  <script>
    const grid = document.getElementById('grid');
    const progress = document.getElementById('progressBar');
    const footer = document.getElementById('footerNote');

    const seen = new Map();
    let scanning = false;

    function updateProgress(done, total) {
      const pct = total ? Math.round((done / total) * 100) : 0;
      progress.style.width = pct + '%';
    }

    function render(info) {
      const id = 'dev-' + info.ip.replaceAll('.', '-');
      if (document.getElementById(id)) return;
      const el = document.createElement('div'); el.className = 'dev'; el.id = id;
      const h = document.createElement('h3'); h.textContent = info.name || 'OBEGRÄNSAD-X'; el.appendChild(h);
      const kv1 = document.createElement('div'); kv1.className='kv'; kv1.textContent = 'IP: ' + info.ip; el.appendChild(kv1);
      if (info.version) { const kv2 = document.createElement('div'); kv2.className='kv'; kv2.textContent = 'Version: ' + info.version; el.appendChild(kv2); }
      if (info.ssid) { const kv3 = document.createElement('div'); kv3.className='kv'; kv3.textContent = 'WLAN: ' + info.ssid; el.appendChild(kv3); }
      const chip = document.createElement('div'); chip.className='chip'; chip.innerHTML = '<span class="dot"></span> erreichbar'; el.appendChild(chip);
      const row = document.createElement('div'); row.className='row';
      const target = info.hostname ? info.hostname : info.ip;
      const a1 = document.createElement('a'); a1.className='btn btn-primary'; a1.href = `http://${target}/local`; a1.textContent='Lokale UI'; a1.target='_blank';
      const a2 = document.createElement('a'); a2.className='btn btn-secondary'; a2.href = `https://zenonik.github.io/obegraensad-x/?device=http://${target}&insecure=1`; a2.textContent='In der Oberfläche öffnen'; a2.target='_self';
      row.appendChild(a2); row.appendChild(a1);
      el.appendChild(row);
      grid.appendChild(el);
    }

    async function fetchWithTimeout(resource, options = {}) {
      const { timeoutMs = 1500, ...opts } = options;
      const controller = new AbortController();
      const id = setTimeout(() => controller.abort(), timeoutMs);
      try {
        const res = await fetch(resource, { ...opts, signal: controller.signal, cache: 'no-store' });
        clearTimeout(id);
        return res;
      } catch (e) {
        clearTimeout(id);
        throw e;
      }
    }

    async function checkIp(ipOrHost) {
      try {
        const res = await fetchWithTimeout(`http://${ipOrHost}/api/status`, { timeoutMs: 1500 });
        if (!res.ok) throw new Error('HTTP ' + res.status);
        const data = await res.json();
        return {
          ip: ipOrHost,
          name: data.device || 'OBEGRÄNSAD-X',
          version: data.version || undefined,
          ssid: data.ssid || undefined,
          hostname: (data.hostname ? `${data.hostname}.local` : undefined),
        };
      } catch (_) {
        // try image ping fallback
        const ok = await new Promise((resolve) => {
          const img = new Image();
          const t = setTimeout(() => { img.onload = img.onerror = null; resolve(false); }, 1600);
          img.onload = () => { clearTimeout(t); img.onload = img.onerror = null; resolve(true); };
          img.onerror = () => { clearTimeout(t); img.onload = img.onerror = null; resolve(false); };
          img.src = `http://${ipOrHost}/api/healthz.gif?ts=${Date.now()}-${Math.random().toString(16).slice(2)}`;
        });
        return ok ? { ip: ipOrHost, name: 'OBEGRÄNSAD-X' } : null;
      }
    }

    function getAutoSubnets() { return ['192.168.0', '192.168.1', '10.0.0', '10.1.1']; }

    async function scan(prefix, onProgress) {
      const concurrency = 32; let current = 0; let done = 0; const total = 254;
      return new Promise((resolve) => {
        const next = async () => {
          if (current >= 254) return;
          const i = ++current; const ip = `${prefix}.${i}`;
          try {
            const info = await checkIp(ip);
            if (info && !seen.has(ip)) { seen.set(ip, info); render(info); }
          } catch {}
          finally {
            done++; onProgress(done, total);
            if (current < 254) next(); else if (done >= total) resolve();
          }
        };
        for (let k = 0; k < Math.min(concurrency, total); k++) next();
      });
    }

    async function runScan() {
      if (scanning) return; scanning = true; grid.innerHTML = ''; seen.clear(); updateProgress(0,1);
      footer.textContent = 'Scanne lokales Netzwerk… dies kann 10–30 Sekunden dauern.';
      const selected = document.getElementById('subnetSelect').value;
      const prefixes = selected === 'auto' ? getAutoSubnets() : [selected];
      let totalPrefixes = prefixes.length; let completed = 0;
      for (const p of prefixes) {
        await scan(p, (d,t) => {
          const totalDone = (completed * 254) + d; const totalAll = totalPrefixes * 254;
          updateProgress(totalDone, totalAll);
        });
        completed++;
      }
      if (seen.size === 0) {
        footer.innerHTML = 'Keine Geräte gefunden. Stelle sicher, dass dein PC/Smartphone im selben WLAN ist. Du kannst das Gerät auch direkt unter <span class="warn">http://obegraensad-x.local</span> öffnen (wenn mDNS unterstützt wird).';
      } else {
        footer.textContent = `Fertig – ${seen.size} Gerät(e) gefunden.`;
      }
      scanning = false;
    }

    function addManual() {
      const v = (document.getElementById('manualIp').value || '').trim(); if (!v) return;
      const looksLikeIp = /^\d+\.\d+\.\d+\.\d+$/.test(v);
      const looksLikeHost = /^[a-z0-9-]+(\.[a-z0-9-]+)*$/i.test(v);
      if (!looksLikeIp && !looksLikeHost) return;
      if (seen.has(v)) return;
      checkIp(v).then(info => { if (info) { seen.set(v, info); render(info); } });
    }

    document.getElementById('scanBtn').addEventListener('click', runScan);
    document.getElementById('addBtn').addEventListener('click', addManual);
    footer.textContent = 'Tipp: Öffne diese Seite über http://[IP]/discover von einem https-UI-Link, um die Gerätesuche zu nutzen.';
  </script>
</body>
</html>
)rawliteral";
    return html;
}
void WebServerManager::handleRoot() {
    // If connected to WiFi, redirect to hosted GitHub Pages UI
    if (wifiConnection.isConnected()) {
        // Prefer unique hostname for better UX when multiple devices exist
        String deviceBase = String(WiFi.getHostname());
        String redirectUrl = "https://zenonik.github.io/obegraensad-x/?device=http://" + deviceBase + ".local&insecure=1";
        setCORSHeaders();
        server.sendHeader("Location", redirectUrl, true);
        server.send(302, "text/plain", "");
        return;
    }
    // Otherwise show local (offline) UI
    server.send(200, "text/html", getHTML());
}

void WebServerManager::handleLocal() {
    server.send(200, "text/html", getHTML());
}

void WebServerManager::handleDiscover() {
    server.send(200, "text/html", getDiscoverHTML());
}

void WebServerManager::handleGetSettings() {
    setCORSHeaders();
    String json = "{";
    json += "\"brightness\":" + String(settingsManager.getBrightness()) + ",";
    json += "\"mode\":" + String(settingsManager.getDisplayMode()) + ",";
    json += "\"city\":\"" + settingsManager.getCity() + "\"";
    json += "}";
    
    server.send(200, "application/json", json);
}

void WebServerManager::handleSaveSettings() {
    setCORSHeaders();
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
    setCORSHeaders();
    String json = "{";
    json += "\"ssid\":\"" + wifiConnection.getSSID() + "\",";
    json += "\"ip\":\"" + wifiConnection.getIP() + "\",";
    json += "\"rssi\":" + String(wifiConnection.getRSSI()) + ",";
    json += "\"time\":\"" + timeManager.getTimeString() + "\",";
    json += "\"date\":\"" + timeManager.getDateString() + "\",";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"device\":\"OBEGRÄNSAD-X\",";
    json += "\"hostname\":\"" + String(WiFi.getHostname()) + "\",";
    json += "\"version\":\"" + String(CURRENT_VERSION) + "\"";
    json += "}";
    
    server.send(200, "application/json", json);
}

void WebServerManager::handleRestart() {
    setCORSHeaders();
    server.send(200, "text/plain", "Restarting...");
    delay(1000);
    ESP.restart();
}

void WebServerManager::handleReset() {
    setCORSHeaders();
    server.send(200, "text/plain", "Resetting WiFi...");
    delay(1000);
    wifiConnection.reset();
}

void WebServerManager::handleNotFound() {
    if (server.method() == HTTP_OPTIONS) {
        handleOptions();
        return;
    }
    setCORSHeaders();
    if (!wifiConnection.isConnected()) {
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    } else {
        server.send(404, "text/plain", "404: Seite nicht gefunden");
    }
}

void WebServerManager::handleOTAUpdate() {
    setCORSHeaders();
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

void WebServerManager::handleOptions() {
    setCORSHeaders();
    server.send(204);
}

void WebServerManager::handlePing() {
    // Small GIF fingerprint for discovery from HTTPS UIs via <img>
    setCORSHeaders();
    server.send_P(200, "image/gif", reinterpret_cast<const char*>(OBEGRAENSAD_PING_GIF), sizeof(OBEGRAENSAD_PING_GIF));
}

void WebServerManager::handleHealth() {
    // Lightweight text health endpoint (ad-blocker friendly)
    setCORSHeaders();
    server.send(200, "text/plain", "ok");
}

void WebServerManager::setCORSHeaders() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    // For Chrome Private Network Access preflights from secure contexts
    server.sendHeader("Access-Control-Allow-Private-Network", "true");
    server.sendHeader("Access-Control-Max-Age", "600");
}
