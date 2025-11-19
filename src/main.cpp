#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "wifi_manager.h"
#include "time_manager.h"
#include "settings_manager.h"
#include "web_server_manager.h"
#include "weather_manager.h"
#include "game_of_life.h"
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClient.h>
#include <Update.h>
#include "version.h"
#include "pong.h"
#include "matrix_rain.h"
#include <math.h>

// ======================================================
// 🧩 GLOBALS
// ======================================================
GameOfLife life(display);
Pong pong;
MatrixRain matrixRain(display);

int previousMode = -1;
unsigned long lastDisplayUpdate = 0;
unsigned long lastButtonCheck = 0;
unsigned long lastWeatherUpdate = 0;

bool lastButtonState = HIGH;
unsigned long lastPress = 0;

bool weatherToggle = false;
unsigned long lastWeatherToggle = 0;

uint8_t autoSubMode = 0;
unsigned long lastAutoSwitch = 0;

const unsigned long AUTO_SWITCH_INTERVAL = 20000; // 20s
const unsigned long MIN_VIEW_TIME = 5000;

// ======================================================
// 🧠 FUNCTION PROTOTYPES
// ======================================================
void handleButton();
void updateDisplay();
void drawTimeView(uint8_t h, uint8_t m);
void drawSecondsView(uint8_t s);
void drawDateView(uint8_t day, uint8_t month);
void updateBrightness();
void checkWiFi();
void updateWeather();
void performOTAUpdate();

// ======================================================
// 🧩 SETUP
// ======================================================
void setup()
{
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== OBEGRÄNSAD-X ESP32 " + String(CURRENT_VERSION) + " ===");

    settingsManager.begin();
    display.begin();
    display.setBrightness(settingsManager.getBrightness());

    pinMode(P_KEY, INPUT_PULLUP);

    display.clear();
    display.drawText2x2("WIFI");
    display.update();

    if (!wifiConnection.begin())
    {
        Serial.println("[WiFi] Verbindung fehlgeschlagen! Neustart in 5s...");
        delay(5000);
        ESP.restart();
    }

    display.animateCheckmark();

    timeManager.begin();
    // Initialisiere Wetter mit gespeicherter Stadt
    weatherManager.begin(settingsManager.getCity());
    webServer.begin();

    Serial.println("[System] Bereit unter: http://" + wifiConnection.getIP());
    delay(300);

    // Restore previous mode
    Serial.printf("[Settings] Gespeicherter Modus: %d\n", settingsManager.getDisplayMode());
    updateDisplay();

    // Initialize Game of Life
    life.begin(250);
}

// ======================================================
// 🔁 LOOP
// ======================================================
void loop()
{
    webServer.handleClient();

    // Check button every 50ms
    if (millis() - lastButtonCheck >= 50)
    {
        lastButtonCheck = millis();
        handleButton();
    }

    // Update display every second
    if (millis() - lastDisplayUpdate >= 1000)
    {
        lastDisplayUpdate = millis();
        timeManager.update();
        updateDisplay();
    }

    // Update weather every 10 minutes
    if (millis() - lastWeatherUpdate > 10UL * 60UL * 1000UL)
    {
        lastWeatherUpdate = millis();
        updateWeather();
    }

    // Check WiFi connection
    checkWiFi();

    // Handle brightness changes
    updateBrightness();

    // Run Game of Life animation
    if (life.isRunning())
    {
        life.update();
    }
    else if (pong.isRunning())
    {
        pong.update();
    }
    else if (matrixRain.isRunning())
    {
        matrixRain.update();
    }

    delay(10);
}

// ======================================================
// 🔘 BUTTON HANDLER
// ======================================================
void handleButton()
{
    bool currentState = digitalRead(P_KEY);

    // Knopf gedrückt
    if (currentState == LOW && lastButtonState == HIGH)
    {
        lastPress = millis();
    }

    // Knopf wird gehalten
    if (currentState == LOW && (millis() - lastPress) >= 5000)
    {
        Serial.println("[Main] 5s langer Druck erkannt → OTA-Update starten");
        performOTAUpdate();
        lastPress = millis() + 10000; // debounce nach OTA
    }

    // Kurzer Druck: Modus wechseln
    if (currentState == HIGH && lastButtonState == LOW && (millis() - lastPress) < 5000)
    {
        Serial.println("[Main] Kurzer Druck – Modus wechseln");
        uint8_t newMode = (settingsManager.getDisplayMode() + 1) % (DISPLAYMODES + 1);
        settingsManager.setDisplayMode(newMode);
        updateDisplay();
    }

    lastButtonState = currentState;
}

// ======================================================
// 🖥️ DISPLAY UPDATE
// ======================================================
void updateDisplay()
{
    uint8_t mode = settingsManager.getDisplayMode();
    uint8_t h = timeManager.getHour();
    uint8_t m = timeManager.getMinute();
    uint8_t s = timeManager.getSecond();

    if (mode != previousMode)
    {
        if (previousMode == 5 && life.isRunning())
        {
            life.stop();
            Serial.println("[GameOfLife] Animation gestoppt");
        }
        else if (previousMode == 6 && pong.isRunning())
        {
            pong.stop();
        }
        else if (previousMode == 8 && matrixRain.isRunning())
        {
            matrixRain.stop();
        }
        previousMode = mode;
    }

    // Weather toggle every 5s in manual mode
    if (mode == 3 && millis() - lastWeatherToggle > 5000)
    {
        weatherToggle = !weatherToggle;
        lastWeatherToggle = millis();
    }

    Serial.printf("[Display] Mode %d | %02d:%02d:%02d\n", mode, h, m, s);

    switch (mode)
    {
    case 0:
        drawTimeView(h, m);
        break; // 🕒 Uhrzeit
    case 1:
        drawSecondsView(s);
        break; // ⏱️ Sekunden
    case 2:
        drawDateView(timeManager.getDay(), timeManager.getMonth());
        break; // 📅 Datum

    case 3: // 🌤️ Wetteranzeige
        if (weatherToggle)
        {
            display.drawWeather(weatherManager.getTemperature(),
                                weatherManager.getCondition() + "_icon",
                                WeatherMode::MODE_ICON);
        }
        else
        {
            display.drawWeather(weatherManager.getTemperature(),
                                weatherManager.getCondition(),
                                WeatherMode::MODE_TEXT);
        }
        break;

    case 4:
    { // 🔁 Automatikmodus Zeit
        bool inSecondsWindow = (s >= 55 && s <= 58) || (s >= 25 && s <= 29);
        if (inSecondsWindow)
            drawSecondsView(s);
        else
            drawTimeView(h, m);
        break;
    }

    case 5: // 🧬 Game of Life
        if (!life.isRunning())
        {
            life.spawnGlider(3, 3);
            life.randomize(30);
            life.start();
            Serial.println("[GameOfLife] Animation gestartet");
        }
        break;

    case 6: // 🏓 Pong
        if (!pong.isRunning())
            pong.start();
        break;

    case 7: // 📶 WiFi Signal
    {
        int rssi = wifiConnection.getRSSI();
        int clamped = constrain(rssi, -90, -30);

        // Map RSSI to number of arcs (0..3). Dot is always drawn.
        int arcs = 0;
        if (clamped > -80) arcs = 1;     // weak
        if (clamped > -67) arcs = 2;     // medium
        if (clamped > -55) arcs = 3;     // strong

        const int cx = 8;   // center x
        const int cy = 15;  // baseline y (bottom)

        auto drawArc = [&](int radius) {
            for (int dx = -radius; dx <= radius; ++dx) {
                int x = cx + dx;
                if (x < 0 || x > 15) continue;
                float inside = (float)(radius * radius - dx * dx);
                if (inside < 0.0f) continue;
                int y = cy - (int)lroundf(sqrtf(inside));
                if (y >= 0 && y < 16) display.setPixel((uint8_t)x, (uint8_t)y, true);
            }
        };

        display.clear();
        // Center dot
        display.setPixel(cx, cy, true);
        // Arcs from inner to outer
        if (arcs >= 1) drawArc(3);
        if (arcs >= 2) drawArc(6);
        if (arcs >= 3) drawArc(9);
        display.update();
        break;
    }

    case 8: // 💚 Matrix Rain
        if (!matrixRain.isRunning()) matrixRain.start();
        break;

    case 9: // ⚫ Display aus (immer letzter Modus)
    default:
        display.clear();
        display.update();
        break;
    }
}

// ======================================================
// 🧱 DRAW HELPERS
// ======================================================
void drawTimeView(uint8_t h, uint8_t m)
{
    display.drawTime(h, m);
}

void drawSecondsView(uint8_t s)
{
    display.clear();
    display.drawDigit(s / 10, 2, 5);
    display.drawDigit(s % 10, 9, 5);
    display.update();
}

void drawDateView(uint8_t day, uint8_t month)
{
    display.clear();
    display.drawDigit(day / 10, 2, 0);
    display.drawDigit(day % 10, 9, 0);
    display.setPixel(15, 6, true);
    display.setPixel(15, 15, true);
    display.drawDigit(month / 10, 2, 9);
    display.drawDigit(month % 10, 9, 9);
    display.update();
}

// ======================================================
// 🌤️ WEATHER UPDATE (Async Task)
// ======================================================
void updateWeather()
{
    xTaskCreatePinnedToCore([](void *)
                            {
        weatherManager.update(true);
        vTaskDelete(NULL); }, "WeatherUpdateTask", 8192, NULL, 1, NULL, 1);
}

// ======================================================
// 📡 WIFI & BRIGHTNESS HELPERS
// ======================================================
void checkWiFi()
{
    if (!wifiConnection.isConnected())
    {
        display.clear();
        display.drawText2x2("WIFI");
        display.update();
        delay(5000);
    }
}

void updateBrightness()
{
    static uint8_t lastBrightness = 255;
    uint8_t current = settingsManager.getBrightness();

    if (current != lastBrightness)
    {
        display.setBrightness(current);
        lastBrightness = current;
        Serial.printf("[Display] Neue Helligkeit übernommen: %d\n", current);
    }
}

// ======================================================
// 🔄 OTA UPDATE HANDLER
// ======================================================
void performOTAUpdate()
{
    Serial.println("[OTA] Firmware-Update angefordert (Web API)");

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
        display.animateCheckmark();
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
