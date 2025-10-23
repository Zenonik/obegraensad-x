#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

// ============================================================
// Display.h
// - Steuerung eines 16x16 LED-Matrix-Displays
// - Enthält Text-, Zeit-, Animation- und Wetterdarstellung
// ============================================================

// 🌦️ Darstellungsmodus für Wetteranzeige
enum class WeatherMode : uint8_t {
    MODE_ICON,   // Wetter-Icons (Sonne, Wolken, Regen, ...)
    MODE_TEXT    // Temperatur als Text (z. B. "23°")
};

class Display {
public:
    Display();

    // 🧭 Grundfunktionen
    void begin();
    void clear();
    void update();

    // 🔆 Anzeigeeinstellungen
    void setBrightness(uint8_t brightness);
    void setPixel(uint8_t x, uint8_t y, bool state);

    // 🔢 Zeichnen von Zeichen & Text
    void drawDigit(uint8_t digit, uint8_t x, uint8_t y);
    void drawCharacter(uint8_t index, uint8_t x, uint8_t y);
    void drawText2x2(const String& text);
    void drawText(const char* text);
    void drawTime(uint8_t hour, uint8_t minute);

    // 🎞️ Tests & Animationen
    void startupAnimation();
    void lineAnimation();
    void circleAnimation();
    void drawCheckmark();
    void animateCheckmark();

    // 🌤️ Wetteranzeige (Temperatur + Icon/Text)
    void drawWeather(float temp, const String& cond, WeatherMode mode);

    // 🔄 Asynchrone Animation (z. B. Hintergrundeffekte)
    void startAsyncAnimation();
    void stopAsyncAnimation();
    void handleAsyncAnimation();

    // 🧪 Debug-/Testfunktionen
    void drawScrollingText(const char* text, int offset);
    void testSinglePixel(uint8_t x, uint8_t y);
    void simpleTest();
    void testPixelMapping();

private:
    // 🧩 Interner Framebuffer (Helligkeitswerte 0–255)
    uint8_t framebuffer[16][16];
    uint8_t brightness;

    // 🧠 Interner Zustand für Animationen
    bool animationActive;
    unsigned long animationFrame;

    // ⚙️ Low-Level-Methoden
    void shiftOut();
    void latch();
    int mapToHardwareIndex(uint8_t x, uint8_t y);

    // 🔄 Interne Hilfsanimation
    void fadeOutCircle();
};

// 🌍 Globale Instanz
extern Display display;

#endif // DISPLAY_H
