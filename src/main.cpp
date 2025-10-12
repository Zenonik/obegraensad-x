#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "wifi_manager.h"
#include "time_manager.h"
#include "settings_manager.h"
#include "web_server_manager.h"
#include "weather_manager.h" // 🌤️ Wetter-Feature

unsigned long lastDisplayUpdate = 0;
unsigned long lastButtonCheck = 0;
bool lastButtonState = HIGH;
unsigned long lastWeatherUpdate = 0;

// 🔁 Zusatz: Wetteranzeige umschalten (Temp/Icon)
static bool weatherToggle = false;
static unsigned long lastWeatherToggle = 0;

void handleButton();
void updateDisplay();

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== OBEGRÄNSAD-X ESP32 v1.1 ===\n");

    settingsManager.begin();
    display.begin();

    pinMode(P_KEY, INPUT_PULLUP);

    display.clear();
    display.drawText("WIFI");
    display.update();

    if (!wifiConnection.begin()) {
        Serial.println("WiFi failed! Restarting...");
        delay(5000);
        ESP.restart();
    }

    timeManager.begin();
    weatherManager.begin(CITY);
    webServer.begin();

    Serial.println("Ready: http://" + wifiConnection.getIP());
    delay(500);
    updateDisplay();
}

void loop() {
    webServer.handleClient();

    if (millis() - lastButtonCheck >= 50) {
        lastButtonCheck = millis();
        handleButton();
    }

    if (millis() - lastDisplayUpdate >= 1000) {
        lastDisplayUpdate = millis();
        timeManager.update();
        updateDisplay();
    }

    if (millis() - lastWeatherUpdate > 10UL * 60UL * 1000UL) { // alle 10 Minuten
        lastWeatherUpdate = millis();
        // Wetterabruf in kurzem Task, damit Loop nicht hängt
        xTaskCreatePinnedToCore([](void*) {
            weatherManager.update();
            vTaskDelete(NULL);
        }, "WeatherUpdateTask", 8192, NULL, 1, NULL, 1);
    }

    if (!wifiConnection.isConnected()) {
        display.clear();
        display.drawText("WIFI");
        display.update();
        delay(5000);
    }

    delay(10);
}

void handleButton() {
    bool currentState = digitalRead(P_KEY);

    if (currentState == LOW && lastButtonState == HIGH) {
        // Knopf gedrückt - Modus wechseln
        Serial.println("[Main] Knopf gedrückt - Modus wechseln");
        uint8_t newMode = (settingsManager.getDisplayMode() + 1) % 5; // 🔹 jetzt 7 Modi
        settingsManager.setDisplayMode(newMode);
        updateDisplay();
    }

    lastButtonState = currentState;
}

void updateDisplay() {
    uint8_t mode = settingsManager.getDisplayMode();

    // 🔁 alle 5 Sekunden Wetteransicht wechseln
    if (mode == 3 && millis() - lastWeatherToggle > 5000) {
        weatherToggle = !weatherToggle;
        lastWeatherToggle = millis();
    }

    Serial.printf("[Main] Update Display - Mode: %d, Zeit: %02d:%02d:%02d\n",
                  mode, timeManager.getHour(), timeManager.getMinute(), timeManager.getSecond());

    switch (mode) {
    case 0: // Zeit HH:MM
        display.drawTime(timeManager.getHour(), timeManager.getMinute());
        break;

    case 1: // Sekunden
        display.clear();
        display.drawDigit(timeManager.getSecond() / 10, 2, 5);
        display.drawDigit(timeManager.getSecond() % 10, 9, 5);
        display.update();
        break;

    case 2: // Datum TT.MM
        display.clear();
        display.drawDigit(timeManager.getDay() / 10, 2, 0);
        display.drawDigit(timeManager.getDay() % 10, 9, 0);
        display.setPixel(8, 6, true);
        display.setPixel(9, 6, true);
        display.drawDigit(timeManager.getMonth() / 10, 2, 9);
        display.drawDigit(timeManager.getMonth() % 10, 9, 9);
        display.update();
        break;

    case 3: // 🌤️ Wetteranzeige
        if (weatherToggle) {
            // Pixel-Art-Ansicht (z. B. Sonne/Wolke)
            display.drawWeather(weatherManager.getTemperature(),
                                weatherManager.getCondition() + "_icon", WeatherMode::MODE_ICON);
        } else {
            // normale Temperaturanzeige
            display.drawWeather(weatherManager.getTemperature(),
                                weatherManager.getCondition(), WeatherMode::MODE_TEXT);
        }
        break;

    case 4: // ⛔ Display aus
        display.clear();
        display.update();
        break;
    }
}
