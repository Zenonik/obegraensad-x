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
void showOTAError();

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

        // Map RSSI auf 1..4 Balken (verbunden => mindestens 1 Balken)
        int level = 1;
        if (clamped > -80)
            level = 2; // schwach
        if (clamped > -67)
            level = 3; // mittel
        if (clamped > -55)
            level = 4; // stark

        const uint8_t barWidth = 2;
        const uint8_t barGap = 1;
        const uint8_t barHeights[4] = {3, 6, 9, 12};
        const uint8_t baselineY = 15;
        const uint8_t startX = 2;

        display.clear();
        uint8_t x = startX;
        for (uint8_t i = 0; i < 4; ++i)
        {
            if (i < (uint8_t)level)
            {
                uint8_t h = barHeights[i];
                for (uint8_t dx = 0; dx < barWidth; ++dx)
                    for (uint8_t y = baselineY - h + 1; y <= baselineY; ++y)
                        display.setPixel(x + dx, y, true);
            }
            x += barWidth + barGap;
        }
        display.update();
        break;
    }

    case 8: // 💚 Matrix Rain
        if (!matrixRain.isRunning())
            matrixRain.start();
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
    Serial.println("[OTA] Button OTA gestartet");

    display.clear();
    display.update();

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    if (!http.begin(client, OTA_FIRMWARE_URL))
    {
        showOTAError();
        return;
    }

    int code = http.GET();
    if (code != HTTP_CODE_OK)
    {
        Serial.printf("[OTA] HTTP Fehler %d\n", code);
        http.end();
        showOTAError();
        return;
    }

    int total = http.getSize();
    Serial.printf("[OTA] Firmwaregröße: %d Bytes\n", total);

    if (total <= 0)
    {
        Serial.println("[OTA] Ungültige Firmwaregröße");
        http.end();
        showOTAError();
        return;
    }

    int lastDisplayedProgress = -1;

    display.clear();
    display.drawDigit(0, 7, 4);
    display.update();

    if (!Update.begin(total))
    {
        Serial.printf("[OTA] Update.begin fehlgeschlagen: %s\n", Update.errorString());
        http.end();
        showOTAError();
        return;
    }

    Update.onProgress([total, &lastDisplayedProgress](size_t progress, size_t)
                      {
        int percent = (progress * 100) / total;
        
        int progressStep = percent / 10;
        if (progressStep != lastDisplayedProgress) {
            lastDisplayedProgress = progressStep;
            Serial.printf("[OTA] Fortschritt: %d%%\n", percent);
            
            display.clear();
            
            // Draw percent digits using available drawDigit() function
            // Position: centered on 16x16 display
            if (percent >= 100) {
                // "100" - three digits
                display.drawDigit(1, 1, 4);   // "1" at x=1
                display.drawDigit(0, 6, 4);   // "0" at x=6
                display.drawDigit(0, 11, 4);  // "0" at x=11
            } else if (percent >= 10) {
                // "10-99" - two digits
                display.drawDigit(percent / 10, 3, 4);      // tens digit
                display.drawDigit(percent % 10, 9, 4);      // ones digit
            } else {
                // "0-9" - one digit, centered
                display.drawDigit(percent, 7, 4);
            }
            
            display.update();
        } });

    WiFiClient *stream = http.getStreamPtr();

    size_t written = Update.writeStream(*stream);

    display.clear();
    display.drawDigit(1, 1, 4);
    display.drawDigit(0, 6, 4);
    display.drawDigit(0, 11, 4);
    display.update();

    Serial.printf("[OTA] Geschrieben: %d / %d Bytes\n", written, total);

    if (written != total)
    {
        Serial.printf("[OTA] Unvollständiger Download: %d von %d Bytes\n", written, total);
        Update.abort();
        http.end();
        showOTAError();
        return;
    }

    if (!Update.end(true))
    {
        Serial.printf("[OTA] Update.end fehlgeschlagen: %s\n", Update.errorString());
        http.end();
        showOTAError();
        return;
    }

    http.end();

    Serial.printf("[OTA] Update abgeschlossen (%d Bytes)\n", written);
    display.clear();
    display.animateCheckmark();
    display.update();
    delay(1500);
    ESP.restart();
}

void showOTAError()
{
    display.clear();
    display.drawText2x2("ERR");
    display.update();
    delay(1500);
}