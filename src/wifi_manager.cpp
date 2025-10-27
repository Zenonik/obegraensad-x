#include "wifi_manager.h"
#include "config.h"
#include "display.h"
#include "version.h"
#include <ESPmDNS.h>

WiFiConnectionManager wifiConnection;

WiFiConnectionManager::WiFiConnectionManager() : connected(false) {}

void WiFiConnectionManager::onConfigMode(WiFiManager *manager) {
    Serial.println("\n=================================");
    Serial.println("Captive Portal Modus aktiv!");
    Serial.println("SSID: " + String(AP_NAME));
    Serial.println("Passwort: " + String(AP_PASSWORD));
    Serial.println("IP: " + WiFi.softAPIP().toString());
    Serial.println("=================================\n");
    
    // Zeige "WIFI" auf Display
    display.clear();
    display.drawText2x2("WIFI");
    display.update();
}

bool WiFiConnectionManager::begin() {
    Serial.println("Starte WiFi-Verbindung...");
    
    // WiFiManager Konfiguration
    wifiManager.setConfigPortalTimeout(180); // 3 Minuten
    wifiManager.setAPCallback(onConfigMode);
    wifiManager.setConnectTimeout(30);
    wifiManager.setDebugOutput(true);
    
    // Hostname setzen (ASCII, kleingeschrieben für mDNS)
    WiFi.setHostname("obegraensad-x");
    
    // Versuche Verbindung oder starte Captive Portal
    connected = wifiManager.autoConnect(AP_NAME, AP_PASSWORD);
    
    if (connected) {
        Serial.println("\n=================================");
        Serial.println("WiFi erfolgreich verbunden!");
        Serial.println("SSID: " + WiFi.SSID());
        Serial.println("IP: " + WiFi.localIP().toString());
        Serial.println("Signal: " + String(WiFi.RSSI()) + " dBm");
        Serial.println("Hostname: obegraensad-x.local");
        Serial.println("=================================\n");

        // mDNS (Bonjour) aktivieren, damit http://obegraensad-x.local erreichbar ist
        String mdnsName = String(WiFi.getHostname());
        mdnsName.toLowerCase();
        if (!MDNS.begin(mdnsName.c_str())) {
            Serial.println("[mDNS] Start fehlgeschlagen");
        } else {
            MDNS.addService("http", "tcp", WEB_SERVER_PORT);
            MDNS.addServiceTxt("http", "tcp", "id", "obegraensad-x");
            MDNS.addServiceTxt("http", "tcp", "ver", CURRENT_VERSION);
            Serial.println("[mDNS] Aktiv: http://" + mdnsName + ".local");
        }
    } else {
        Serial.println("WiFi-Verbindung fehlgeschlagen!");
    }
    
    return connected;
}

bool WiFiConnectionManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiConnectionManager::getSSID() {
    return WiFi.SSID();
}

String WiFiConnectionManager::getIP() {
    return WiFi.localIP().toString();
}

int WiFiConnectionManager::getRSSI() {
    return WiFi.RSSI();
}

void WiFiConnectionManager::reset() {
    wifiManager.resetSettings();
    ESP.restart();
}
