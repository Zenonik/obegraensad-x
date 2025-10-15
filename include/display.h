#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

// Beispiel-Definition für WeatherMode
enum WeatherMode {
    MODE_ICON,
    MODE_TEXT,
};

class Display {
public:
    Display();

    void begin();
    void clear();
    void setBrightness(uint8_t brightness);
    void setPixel(uint8_t x, uint8_t y, bool state);
    void update();
    void drawDigit(uint8_t digit, uint8_t x, uint8_t y);
    void drawCharacter(uint8_t index, uint8_t x, uint8_t y);
    void drawText2x2(const String& text);
    void drawTime(uint8_t hour, uint8_t minute);
    void drawText(const char* text);
    void startupAnimation();
    void drawScrollingText(const char* text, int offset);
    void testSinglePixel(uint8_t x, uint8_t y);
    void simpleTest();
    void testPixelMapping();
    void lineAnimation();
    void circleAnimation();
    void drawWeather(float temp, const String& cond, WeatherMode mode); // Original

    // 🌀 Neue asynchrone Startup-Animation
    void startAsyncAnimation();
    void stopAsyncAnimation();
    void handleAsyncAnimation();

private:
    uint8_t framebuffer[16][16];
    uint8_t brightness;

    void shiftOut();
    void latch();
    int mapToHardwareIndex(uint8_t x, uint8_t y);

    // ⚙️ Statusvariablen für Animation
    bool animationActive = false;
    unsigned long animationFrame = 0;
};

extern Display display;

#endif
