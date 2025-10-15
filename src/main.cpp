#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "wifi_manager.h"
#include "time_manager.h"
#include "settings_manager.h"
#include "web_server_manager.h"
#include "weather_manager.h" // 🌤️ Wetter-Feature
#include "game_of_life.h"

GameOfLife life(display);

int previousMode = -1;

// --- Zeitmanagement ---
unsigned long lastDisplayUpdate = 0;
unsigned long lastButtonCheck = 0;
unsigned long lastWeatherUpdate = 0;

// --- Buttonstatus ---
bool lastButtonState = HIGH;
static unsigned long lastPress = 0;

// --- Wetterumschaltung ---
static bool weatherToggle = false;
static unsigned long lastWeatherToggle = 0;

// --- Automatikmodus ---
static uint8_t autoSubMode = 0;           // 0=Zeit, 1=Datum, 2=Wetter
static unsigned long lastAutoSwitch = 0;
const unsigned long AUTO_SWITCH_INTERVAL = 20000; // alle 20s wechseln
const unsigned long MIN_VIEW_TIME = 5000;

// --- Funktionsprototypen ---
void handleButton();
void updateDisplay();
void drawTimeView(uint8_t h, uint8_t m)        { display.drawTime(h, m); }
void drawSecondsView(uint8_t s)                {
    display.clear();
    display.drawDigit(s / 10, 2, 5);
    display.drawDigit(s % 10, 9, 5);
    display.update();
}
void drawDateView(uint8_t day, uint8_t month) {
    display.clear();
    display.drawDigit(day / 10, 2, 0);
    display.drawDigit(day % 10, 9, 0);
    display.setPixel(8, 6, true);
    display.setPixel(9, 6, true);
    display.drawDigit(month / 10, 2, 9);
    display.drawDigit(month % 10, 9, 9);
    display.update();
}

// ======================================================
// 🧩 SETUP
// ======================================================
void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== OBEGRÄNSAD-X ESP32 v1.2 ===\n");

    settingsManager.begin();
    display.begin();
    uint8_t brightness = settingsManager.getBrightness();
    display.setBrightness(brightness);
    Serial.printf("[Display] Initiale Helligkeit: %d\n", brightness);
    pinMode(P_KEY, INPUT_PULLUP);

    display.clear();
    display.drawText2x2("WIFI");
    display.update();

    if (!wifiConnection.begin()) {
        Serial.println("[WiFi] Verbindung fehlgeschlagen! Neustart in 5s...");
        delay(5000);
        ESP.restart();
    }

    display.drawCheckmark();

    timeManager.begin();
    weatherManager.begin(CITY);
    webServer.begin();

    Serial.println("[System] Bereit unter: http://" + wifiConnection.getIP());
    delay(300);

    // ✅ Gespeicherten Modus auslesen und anwenden
    uint8_t savedMode = settingsManager.getDisplayMode();
    Serial.printf("[Settings] Gespeicherter Modus: %d\n", savedMode);
    updateDisplay();

    // Game of Life
    life.begin(250);
}

// ======================================================
// 🔁 LOOP
// ======================================================
void loop() {
    webServer.handleClient();

    // Buttonprüfung
    if (millis() - lastButtonCheck >= 50) {
        lastButtonCheck = millis();
        handleButton();
    }

    // Displayupdate jede Sekunde
    if (millis() - lastDisplayUpdate >= 1000) {
        lastDisplayUpdate = millis();
        timeManager.update();
        updateDisplay();
    }

    // Wetterupdate alle 10 Minuten
    if (millis() - lastWeatherUpdate > 10UL * 60UL * 1000UL) {
        lastWeatherUpdate = millis();
        xTaskCreatePinnedToCore([](void*) {
            weatherManager.update();
            vTaskDelete(NULL);
        }, "WeatherUpdateTask", 8192, NULL, 1, NULL, 1);
    }

    // WiFi-Check
    if (!wifiConnection.isConnected()) {
        display.clear();
        display.drawText2x2("WIFI");
        display.update();
        delay(5000);
    }

    // Helligkeit
    static uint8_t lastBrightness = 255;
    uint8_t currentBrightness = settingsManager.getBrightness();
    if (currentBrightness != lastBrightness) {
        display.setBrightness(currentBrightness);
        lastBrightness = currentBrightness;
        Serial.printf("[Display] Neue Helligkeit übernommen: %d\n", currentBrightness);
    }

    // Game of Life Update
    if (life.isRunning()) {
        life.update();
    }

    delay(10);
}

// ======================================================
// 🔘 BUTTON HANDLER
// ======================================================
void handleButton() {
    bool currentState = digitalRead(P_KEY);

    if (currentState == LOW && lastButtonState == HIGH && millis() - lastPress > 300) {
        lastPress = millis();
        Serial.println("[Main] Knopf gedrückt – Modus wechseln");

        // 🔁 Each Case = One Modie
        uint8_t newMode = (settingsManager.getDisplayMode() + 1) % (DISPLAYMODES+1);
        settingsManager.setDisplayMode(newMode); // ✅ wird gespeichert!
        updateDisplay();
    }

    lastButtonState = currentState;
}

// ======================================================
// 🖥️ DISPLAY UPDATE
// ======================================================
void updateDisplay() {
    uint8_t mode = settingsManager.getDisplayMode();
    uint8_t h = timeManager.getHour();
    uint8_t m = timeManager.getMinute();
    uint8_t s = timeManager.getSecond();

    if (mode != previousMode) {
        // ==== optionales Aufräumen ====
        if (previousMode == 5 && life.isRunning()) {
            life.stop();
            Serial.println("[GameOfLife] Animation gestoppt");
        }

        previousMode = mode;  // aktuellen Modus merken
    }

    // Wetterumschaltung im manuellen Modus
    if (mode == 3 && millis() - lastWeatherToggle > 5000) {
        weatherToggle = !weatherToggle;
        lastWeatherToggle = millis();
    }

    Serial.printf("[Display] Mode %d | %02d:%02d:%02d\n", mode, h, m, s);

    switch (mode) {
    // 🔁 Case 0: Uhrzeit
    case 0: drawTimeView(h, m); break;

    // 🔁 Case 1: Sekundenanzeige
    case 1: drawSecondsView(s); break;

    // 🔁 Case 2: Datumsanzeige
    case 2: drawDateView(timeManager.getDay(), timeManager.getMonth()); break;

    // 🔁 Case 3: Wetteranzeige
    case 3:
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
    break;

    // 🔁 Case 4: Automatikmodus Zeit 
    case 4: { 
        bool inSecondsWindow = (s >= 55 && s <= 58) || (s >= 25 && s <= 29);

        if (inSecondsWindow) {
            drawSecondsView(s);
            return; 
        } else {
            drawTimeView(h, m);
            return;
        }
        break;
    }

    // 🔁 Case 5: Game of Life
    case 5:
        if (!life.isRunning()) {
            life.spawnGlider(3, 3); // kleine Rakete in der Ecke
            life.randomize(30);   // etwas Leben
            life.start();
            Serial.println("[GameOfLife] Animation gestartet");
        }
        break;

    // ⛔ Case 6: Display aus
    case 6:
        display.clear();
        display.update();
        break;

    default:
        display.clear();
        display.update();
        break;
    }
}
