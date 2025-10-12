#include "wifi_manager.h"
#include "config.h"
#include "display.h"

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
    display.drawText("WIFI");
    display.update();
}

bool WiFiConnectionManager::begin() {
    Serial.println("Starte WiFi-Verbindung...");
    
    // WiFiManager Konfiguration
    wifiManager.setConfigPortalTimeout(180); // 3 Minuten
    wifiManager.setAPCallback(onConfigMode);
    wifiManager.setConnectTimeout(30);
    wifiManager.setDebugOutput(true);
    
    // Hostname setzen
    WiFi.setHostname("OBEGRAENSAD-X");
    
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
