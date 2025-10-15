#include "weather_manager.h"
#include "wifi_manager.h"
#include <WiFiClientSecure.h>
#include "display.h"

WeatherManager weatherManager;

void WeatherManager::begin(const String& c) {
    city = c;
    update();
}

static String mapCondition(const String& raw) {
    String r = raw;
    r.toLowerCase();

    if (r.indexOf("regen") >= 0 || r.indexOf("rain") >= 0) return "Rain";
    if (r.indexOf("bewölkt") >= 0 || r.indexOf("bedeckt") >= 0 || r.indexOf("cloud") >= 0) return "Cloud";
    if (r.indexOf("sonnig") >= 0 || r.indexOf("klar") >= 0 || r.indexOf("clear") >= 0) return "Clear";
    if (r.indexOf("nebel") >= 0 || r.indexOf("fog") >= 0) return "Fog";
    if (r.indexOf("schnee") >= 0 || r.indexOf("snow") >= 0) return "Snow";
    if (r.indexOf("gewitter") >= 0 || r.indexOf("thunder") >= 0) return "Thunder";
    return "Unknown";
}

void WeatherManager::update() {
    // Alle 30 Minuten aktualisieren
    if (millis() - lastUpdate < 30UL * 60UL * 1000UL && lastUpdate != 0) return;
    lastUpdate = millis();

    if (!wifiConnection.isConnected()) return;

    display.drawText2x2("WTTR");

    String url = "https://wttr.in/" + city + "?format=j1";
    Serial.println("[Weather] Lade: " + url);

    WiFiClientSecure client;
    client.setInsecure();  // kein Zertifikat prüfen

    HTTPClient http;
    if (!http.begin(client, url)) {
        Serial.println("[Weather] HTTP begin() fehlgeschlagen");
        return;
    }

    int code = http.GET();
    if (code != 200) {
        Serial.printf("[Weather] HTTP Fehler: %d\n", code);
        http.end();
        return;
    }

    String payload = http.getString();
    http.end();

    // JSON kann sehr groß sein → größeren Buffer
    DynamicJsonDocument doc(16384);
    DeserializationError err = deserializeJson(doc, payload);

    if (err) {
        Serial.println("[Weather] JSON Parse Error: " + String(err.c_str()));
        return;
    }

    JsonObject current = doc["current_condition"][0];
    if (!current.isNull()) {
        const char* t = current["temp_C"];
        const char* desc = nullptr;

        // bevorzugt deutsche Beschreibung, sonst englische
        if (current["lang_de"][0]["value"]) {
            desc = current["lang_de"][0]["value"];
        } else if (current["weatherDesc"][0]["value"]) {
            desc = current["weatherDesc"][0]["value"];
        }

        if (t) temperature = atof(t);
        if (desc) condition = mapCondition(String(desc));

        Serial.printf("[Weather] %.1f°C, %s\n", temperature, condition.c_str());
        display.drawCheckmark();
    } else {
        Serial.println("[Weather] Kein gültiger current_condition-Block gefunden");
    }
}

float WeatherManager::getTemperature() const { return temperature; }
String WeatherManager::getCondition() const { return condition; }
