#include "display.h"
#include "config.h"
#include "font.h"
#include "settings_manager.h"
#include <Arduino.h>
#include <math.h>

// ======================================================
// Refactored Display.cpp
// Ziel: gleiche API, bessere Struktur, modularisierte Wetter-Icons
// ======================================================

// Externale Singleton-Instanz (wie zuvor)
Display display;

// ------------------------------------------------------
// Hardware LUT ( unverändert, const für Optimierung )
// ------------------------------------------------------
static const uint8_t lut[16][16] PROGMEM = {
    {255, 254, 253, 252, 251, 250, 249, 248, 239, 238, 237, 236, 235, 234, 233, 232},
    {240, 241, 242, 243, 244, 245, 246, 247, 224, 225, 226, 227, 228, 229, 230, 231},
    {207, 206, 205, 204, 203, 202, 201, 200, 223, 222, 221, 220, 219, 218, 217, 216},
    {192, 193, 194, 195, 196, 197, 198, 199, 208, 209, 210, 211, 212, 213, 214, 215},
    {191, 190, 189, 188, 187, 186, 185, 184, 175, 174, 173, 172, 171, 170, 169, 168},
    {176, 177, 178, 179, 180, 181, 182, 183, 160, 161, 162, 163, 164, 165, 166, 167},
    {143, 142, 141, 140, 139, 138, 137, 136, 159, 158, 157, 156, 155, 154, 153, 152},
    {128, 129, 130, 131, 132, 133, 134, 135, 144, 145, 146, 147, 148, 149, 150, 151},
    {127, 126, 125, 124, 123, 122, 121, 120, 111, 110, 109, 108, 107, 106, 105, 104},
    {112, 113, 114, 115, 116, 117, 118, 119, 96, 97, 98, 99, 100, 101, 102, 103},
    {79, 78, 77, 76, 75, 74, 73, 72, 95, 94, 93, 92, 91, 90, 89, 88},
    {64, 65, 66, 67, 68, 69, 70, 71, 80, 81, 82, 83, 84, 85, 86, 87},
    {63, 62, 61, 60, 59, 58, 57, 56, 47, 46, 45, 44, 43, 42, 41, 40},
    {48, 49, 50, 51, 52, 53, 54, 55, 32, 33, 34, 35, 36, 37, 38, 39},
    {15, 14, 13, 12, 11, 10, 9, 8, 31, 30, 29, 28, 27, 26, 25, 24},
    {0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23}
};

// ------------------------------------------------------
// Icon-Templates: Koordinatenlisten für Icons
// - jede Liste ist const und kompakt (x,y-Paare)
// - wenn mehrere Icons die gleiche Cloud-Basis nutzen, referenziert
// ------------------------------------------------------

// Hilfs-Makros für Lesbarkeit beim Definieren
#define P(x,y) x, y

// Cloud base (wird von vielen Icons geteilt)
static const uint8_t cloud_base[][2] PROGMEM = {
    {P(5,3)},{P(6,3)},{P(7,3)},{P(8,3)},{P(9,3)},
    {P(3,4)},{P(4,4)},{P(10,4)},{P(11,4)},{P(12,4)},
    {P(1,5)},{P(2,5)},{P(5,5)},{P(6,5)},{P(13,5)},{P(14,5)},
    {P(0,6)},{P(1,6)},{P(11,6)},{P(12,6)},{P(15,6)},
    {P(1,7)},{P(2,7)},{P(15,7)},{P(2,8)},{P(3,8)},{P(14,8)},
    {P(3,9)},{P(4,9)},{P(5,9)},{P(6,9)},{P(7,9)},{P(8,9)},{P(9,9)},{P(10,9)},{P(11,9)},{P(12,9)},{P(13,9)}
};
static const size_t cloud_base_len = sizeof(cloud_base) / sizeof(cloud_base[0]);

// Cloud outline (leicht abweichend in der alten Clear/Wolken-Templates)
// Für "Cloud" (leicht angepasst aus Ursprungscode)
static const uint8_t cloud_outline[][2] PROGMEM = {
    {P(5,4)},{P(6,4)},{P(7,4)},{P(8,4)},{P(9,4)},
    {P(3,5)},{P(4,5)},{P(10,5)},{P(11,5)},{P(12,5)},
    {P(1,6)},{P(2,6)},{P(5,6)},{P(6,6)},{P(13,6)},{P(14,6)},
    {P(0,7)},{P(1,7)},{P(11,7)},{P(12,7)},{P(15,7)},
    {P(1,8)},{P(2,8)},{P(15,8)},{P(2,9)},{P(3,9)},{P(14,9)},
    {P(3,10)},{P(4,10)},{P(5,10)},{P(6,10)},{P(7,10)},{P(8,10)},{P(9,10)},{P(10,10)},{P(11,10)},{P(12,10)},{P(13,10)}
};
static const size_t cloud_outline_len = sizeof(cloud_outline) / sizeof(cloud_outline[0]);

// Rain drops (für Regen-Icon)
static const uint8_t rain_drops[][2] PROGMEM = {
    {P(5,10)},{P(7,10)},{P(9,10)},
    {P(6,11)},{P(8,11)},{P(10,11)},
    {P(7,12)},{P(9,12)},{P(11,12)}
};
static const size_t rain_drops_len = sizeof(rain_drops) / sizeof(rain_drops[0]);

// Snowflakes
static const uint8_t snowflakes[][2] PROGMEM = {
    {P(5,11)},{P(9,12)},{P(13,11)}
};
static const size_t snowflakes_len = sizeof(snowflakes) / sizeof(snowflakes[0]);

// Lightning bolt (für Thunder)
static const uint8_t lightning[][2] PROGMEM = {
    {P(7,10)},{P(7,11)},{P(8,11)},{P(8,12)},
    {P(9,12)},{P(9,13)},{P(10,13)},{P(10,14)}
};
static const size_t lightning_len = sizeof(lightning) / sizeof(lightning[0]);

// Sun core + rays (Clear)
static const uint8_t sun_core[][2] PROGMEM = {
    {P(6,6)},{P(7,6)},{P(8,6)},{P(9,6)},
    {P(5,7)},{P(6,7)},{P(7,7)},{P(8,7)},{P(9,7)},{P(10,7)},
    {P(5,8)},{P(6,8)},{P(7,8)},{P(8,8)},{P(9,8)},{P(10,8)},
    {P(6,9)},{P(7,9)},{P(8,9)},{P(9,9)}
};
static const size_t sun_core_len = sizeof(sun_core) / sizeof(sun_core[0]);

static const uint8_t sun_rays[][2] PROGMEM = {
    {P(7,4)},{P(8,4)},{P(4,5)},{P(11,5)},
    {P(3,7)},{P(12,7)},{P(4,10)},{P(11,10)},
    {P(7,11)},{P(8,11)},{P(2,7)},{P(13,7)},
    {P(3,5)},{P(12,5)},{P(3,10)},{P(12,10)}
};
static const size_t sun_rays_len = sizeof(sun_rays) / sizeof(sun_rays[0]);

// Fog lines (simple rows)
static const uint8_t fog_lines[][2] PROGMEM = {
    {P(2,8)},{P(3,8)},{P(4,8)},{P(5,8)},{P(6,8)},{P(7,8)},{P(8,8)},{P(9,8)},{P(10,8)},{P(11,8)},{P(12,8)},{P(13,8)},
    {P(2,6)},{P(3,6)},{P(4,6)},{P(5,6)},{P(6,6)},{P(7,6)},{P(8,6)},{P(9,6)},{P(10,6)},{P(11,6)},{P(12,6)},{P(13,6)},
    {P(2,4)},{P(3,4)},{P(4,4)},{P(5,4)},{P(6,4)},{P(7,4)},{P(8,4)},{P(9,4)},{P(10,4)},{P(11,4)},{P(12,4)},{P(13,4)}
};
static const size_t fog_lines_len = sizeof(fog_lines) / sizeof(fog_lines[0]);

#undef P

// ------------------------------------------------------
// Interne Hilfsfunktionen (nicht in Header exportiert)
// ------------------------------------------------------
static inline void drawTemplateFromProgmem(const uint8_t template_xy[][2], size_t len) {
    for (size_t i = 0; i < len; ++i) {
        uint8_t x = pgm_read_byte(&template_xy[i][0]);
        uint8_t y = pgm_read_byte(&template_xy[i][1]);
        display.setPixel(x, y, true);
    }
}

// ------------------------------------------------------
// Konstruktor-ähnliche Initialisierung & Member-Variablen
// ------------------------------------------------------
Display::Display() : brightness(200), animationActive(false), animationFrame(0) {
    clear();
}

// ------------------------------------------------------
// begin(): Pins, brightness from settings, startup animation
// ------------------------------------------------------
void Display::begin() {
    Serial.println("[Display] Initialisiere Display...");

    pinMode(P_EN, OUTPUT);
    pinMode(P_DI, OUTPUT);
    pinMode(P_CLK, OUTPUT);
    pinMode(P_CLA, OUTPUT);

    digitalWrite(P_EN, LOW);
    digitalWrite(P_CLA, LOW);
    digitalWrite(P_CLK, LOW);
    digitalWrite(P_DI, LOW);

    // Brightness aus Settings übernehmen (wie ursprünglich)
    brightness = settingsManager.getBrightness();
    setBrightness(brightness); // <-- aktiv steuern!
    Serial.println("[Display] Übernommene Brightness: " + String(brightness));

    clear();
    update();
    startupAnimation();

    Serial.println("[Display] Initialisierung abgeschlossen");
}

// ------------------------------------------------------
// Framebuffer-Operationen
// ------------------------------------------------------
void Display::clear() {
    memset(framebuffer, 0, sizeof(framebuffer));
}

void Display::setBrightness(uint8_t b) {
    brightness = constrain(b, 0, 255);

    // PWM-Invertierung (0 = hell, 255 = aus)
    uint8_t pwmValue = 255 - brightness;

#if defined(ESP32)
    // ESP32 PWM über LEDC
    ledcAttachPin(P_EN, 0);
    ledcSetup(0, 5000, 8);  // Kanal 0, 5 kHz, 8 Bit
    ledcWrite(0, pwmValue);
#else
    analogWrite(P_EN, pwmValue);
#endif
}

void Display::setPixel(uint8_t x, uint8_t y, bool state) {
    if (x >= 16 || y >= 16) return;
#ifdef ROTATE_DISPLAY
    x = 15 - x;
    y = 15 - y;
#endif
    framebuffer[y][x] = state ? brightness : 0;
}

int Display::mapToHardwareIndex(uint8_t x, uint8_t y) {
    if (x >= 16 || y >= 16) return 0;
    return pgm_read_byte(&lut[y][x]);
}

// ------------------------------------------------------
// Low-level shift / latch / update
// ------------------------------------------------------
void Display::shiftOut() {
    // lokal speichern, um direkten Zugriff auf framebuffer einmalig zu machen
    bool hardwareBuffer[256];
    memset(hardwareBuffer, 0, sizeof(hardwareBuffer));

    for (uint8_t y = 0; y < 16; ++y) {
        for (uint8_t x = 0; x < 16; ++x) {
            int hwIndex = mapToHardwareIndex(x, y);
            hardwareBuffer[hwIndex] = framebuffer[y][x] > 0;
        }
    }

    for (int i = 0; i < 256; ++i) {
        digitalWrite(P_DI, hardwareBuffer[i] ? HIGH : LOW);
        digitalWrite(P_CLK, HIGH);
        delayMicroseconds(4);
        digitalWrite(P_CLK, LOW);
        delayMicroseconds(4);
    }
}

void Display::latch() {
    delayMicroseconds(5);
    digitalWrite(P_CLA, HIGH);
    delayMicroseconds(5);
    digitalWrite(P_CLA, LOW);
}

void Display::update() {
    shiftOut();
    latch();
}

// ------------------------------------------------------
// Zeichnen von Zeichen / Ziffern / Texte
// ------------------------------------------------------
void Display::drawDigit(uint8_t digit, uint8_t x, uint8_t y) {
    if (digit > 9) return;

    for (uint8_t row = 0; row < 7; ++row) {
        uint8_t line = pgm_read_byte(&font5x7[digit][row]);
        for (uint8_t col = 0; col < 5; ++col) {
            if (line & (1 << (4 - col))) setPixel(x + col, y + row, true);
        }
    }
}

void Display::drawCharacter(uint8_t index, uint8_t x, uint8_t y) {
    // index: 0-9 => digits, 10+ => letters (A=10)
    for (uint8_t row = 0; row < 7; ++row) {
        uint8_t line = pgm_read_byte(&font5x7[index][row]);
        for (uint8_t col = 0; col < 5; ++col)
            if (line & (1 << (4 - col))) setPixel(x + col, y + row, true);
    }
}

void Display::drawText2x2(const String &text) {
    clear();
    uint8_t maxChars = min((int)text.length(), 4);
    for (uint8_t i = 0; i < maxChars; ++i) {
        char c = toupper(text[i]);
        uint8_t x = (i % 2 == 0) ? 2 : 9; // links/rechts
        uint8_t y = (i < 2) ? 0 : 9;      // oben/unten

        if (c >= '0' && c <= '9') drawCharacter(c - '0', x, y);
        else if (c >= 'A' && c <= 'Z') drawCharacter(c - 'A' + 10, x, y);
    }
    update();
}

void Display::drawText(const char *text) {
    clear();
    uint8_t x = 1;
    uint8_t y = 2;

    for (const char *p = text; *p; ++p) {
        char c = toupper(*p);
        if (c >= '0' && c <= '9') {
            drawCharacter(c - '0', x, y);
            x += 6;
        } else if (c >= 'A' && c <= 'Z') {
            drawCharacter(c - 'A' + 10, x, y);
            x += 6;
        } else if (c == ' ') {
            x += 4;
        }
    }
    update();
}

void Display::drawTime(uint8_t hour, uint8_t minute) {
    clear();
    drawDigit(hour / 10, 2, 0);
    drawDigit(hour % 10, 9, 0);
    drawDigit(minute / 10, 2, 9);
    drawDigit(minute % 10, 9, 9);
    update();
}

// ------------------------------------------------------
// Animations: async / startup / helpers
// ------------------------------------------------------
void Display::startAsyncAnimation() {
    animationActive = true;
    animationFrame = millis();
}

void Display::stopAsyncAnimation() {
    animationActive = false;
    clear();
    update();
}

void Display::handleAsyncAnimation() {
    if (!animationActive) return;

    unsigned long now = millis();
    if (now - animationFrame <= 60) return;
    animationFrame = now;

    clear();
    for (uint8_t y = 0; y < 16; ++y) {
        for (uint8_t x = 0; x < 16; ++x) {
            float wave = sin((x * 0.4f) + (now / 250.0f)) + cos((y * 0.3f) + (now / 300.0f));
            if (wave + (random(-10, 10) / 10.0f) > 0.8f) setPixel(x, y, true);
        }
    }
    update();
}

void Display::startupAnimation() {
    Serial.println("[Display] Startanimation: Stripe Sweep mit Kreis-Fadeout");

    clear(); update();
    const int speed = 30;

    // Horizontaler Sweep
    for (int step = 0; step <= 16; ++step) {
        clear();
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < step; ++x)
                if ((x + y) % 2 == 0) setPixel(x, y, true);
        setBrightness(150);
        update();
        delay(speed);
    }

    // Diagonale Streifenwelle
    for (int frame = 0; frame < 24; ++frame) {
        clear();
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
                if ((x + y + frame) % 8 < 3) setPixel(x, y, true);
        int b = 120 + (int)(80 * sin(frame * 0.4f));
        setBrightness((uint8_t)b);
        update();
        delay(speed);
    }

    // Vertikale Lichtflut (unten -> oben)
    for (int step = 15; step >= 0; --step) {
        clear();
        for (int y = step; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
                if ((x + y + step) % 3 < 2) setPixel(x, y, true);
        setBrightness(180);
        update();
        delay(speed);
    }

    // Kurzer Aufblitz
    for (int i = 0; i < 2; ++i) {
        setBrightness(255); update(); delay(80);
        setBrightness(100); update(); delay(80);
    }

    // Kreisförmiges Ausblenden
    fadeOutCircle();

    clear(); update();
    Serial.println("[Display] Startanimation abgeschlossen (Kreis-Fadeout)");
}

void Display::fadeOutCircle() {
    const float cx = 7.5f, cy = 7.5f;
    delay(200);
    for (float radius = 0.0f; radius <= 12.0f; radius += 1.0f) {
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x) {
                float dx = x - cx;
                float dy = y - cy;
                float d = sqrtf(dx * dx + dy * dy);
                if (d < radius) setPixel(x, y, false);
            }
        update();
        delay(50);
    }
}

// circleAnimation & lineAnimation preserved, slightly cleaned
void Display::circleAnimation() {
    const float cx = 7.5f, cy = 7.5f;
    for (float r = 0.0f; r <= 12.0f; r += 0.5f) {
        clear();
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
                if (sqrtf((x - cx) * (x - cx) + (y - cy) * (y - cy)) < r) setPixel(x, y, true);
        update();
        delay(30);
    }
    for (float r = 12.0f; r >= 0.0f; r -= 0.5f) {
        clear();
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
                if (sqrtf((x - cx) * (x - cx) + (y - cy) * (y - cy)) < r) setPixel(x, y, true);
        update();
        delay(30);
    }
}

void Display::lineAnimation() {
    for (int step = 0; step <= 16; ++step) {
        clear();
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < step; ++x)
                if ((x + y) % 2 == 0) setPixel(x, y, true);
        setBrightness(150);
        update();
        delay(30);
    }
    for (int step = 15; step >= 0; --step) {
        clear();
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < step; ++x)
                if ((x + y) % 2 == 0) setPixel(x, y, true);
        setBrightness(150);
        update();
        delay(30);
    }
}

// ------------------------------------------------------
// CHECKMARK
// ------------------------------------------------------
void Display::drawCheckmark() {
    clear();
    const uint8_t checkmark[][2] = {
        {3, 8}, {4, 9}, {5, 10}, {6, 9}, {7, 8}, {8, 7}, {9, 6}, {10, 5}, {11, 4}
    };
    for (auto &p : checkmark) setPixel(p[0], p[1], true);
    update();
    delay(500);
}

void Display::animateCheckmark() {
    clear();
    const uint8_t checkmark[][2] = {
        {3, 8}, {4, 9}, {5, 10}, {6, 9}, {7, 8}, {8, 7}, {9, 6}, {10, 5}, {11, 4}
    };
    for (auto &p : checkmark) {
        setPixel(p[0], p[1], true);
        update();
        delay(60);
    }
    for (int i = 0; i < 2; ++i) {
        setBrightness(255); update(); delay(80);
        setBrightness(150); update(); delay(80);
    }

    brightness = settingsManager.getBrightness();
    setBrightness(brightness);
}

// ------------------------------------------------------
// Wetteranzeige (modularisiert)
// - drawWeather(temp, cond, mode)
// - Icons nutzen Templates oben, weniger Duplikation
// ------------------------------------------------------
void Display::drawWeather(float temp, const String &cond, WeatherMode mode) {
    clear();

    if (mode == WeatherMode::MODE_TEXT) {
        int t = roundf(temp);
        bool negative = (t < 0);
        int absT = abs(t);

        // Positionierung: mittig — einstellbar
        if (absT >= 10) {
            // zweistellig
            drawDigit(absT / 10, 2, 4);
            drawDigit(absT % 10, 9, 4);
        } else {
            // einstellig: zentriert
            drawDigit(absT, 7, 4);
        }

        // Grad-Punkt (rechts oben neben der Zahl)
        setPixel(15, 3, true);
        // optional: Minus-Zeichen anzeigen falls negativ
        if (negative) {
            // einfacher Strich links von Zahl
            for (uint8_t x = 3; x <= 11; ++x) { /* Platzhalter, falls du ein Minus Zeichen willst */ }
            // wir zeichnen ein kleines Minus bei x=2,y=5
            setPixel(2,5,true); setPixel(3,5,true); setPixel(4,5,true);
        }

    } else { // MODE_ICON
        // Entscheide Icon anhand cond-String (cases ähnlich wie original)
        if (cond.indexOf("Cloud") >= 0 && cond.indexOf("Rain") < 0 && cond.indexOf("Snow") < 0) {
            // wolken-icon (outline)
            drawTemplateFromProgmem(cloud_outline, cloud_outline_len);
        }
        else if (cond.indexOf("Rain") >= 0) {
            drawTemplateFromProgmem(cloud_base, cloud_base_len);
            drawTemplateFromProgmem(rain_drops, rain_drops_len);
        }
        else if (cond.indexOf("Snow") >= 0) {
            drawTemplateFromProgmem(cloud_base, cloud_base_len);
            drawTemplateFromProgmem(snowflakes, snowflakes_len);
        }
        else if (cond.indexOf("Thunder") >= 0 || cond.indexOf("Storm") >= 0) {
            drawTemplateFromProgmem(cloud_base, cloud_base_len);
            drawTemplateFromProgmem(lightning, lightning_len);
        }
        else if (cond.indexOf("Clear") >= 0 || cond.indexOf("Sunny") >= 0) {
            drawTemplateFromProgmem(sun_core, sun_core_len);
            drawTemplateFromProgmem(sun_rays, sun_rays_len);
        }
        else if (cond.indexOf("Fog") >= 0 || cond.indexOf("Mist") >= 0 || cond.indexOf("Haze") >= 0) {
            // Nebel: mehrere horizontale Linien (leicht versetzt)
            for (size_t i = 0; i < fog_lines_len; ++i) {
                uint8_t x = pgm_read_byte(&fog_lines[i][0]);
                uint8_t y = pgm_read_byte(&fog_lines[i][1]);
                // Streuung: nur manche Pixel
                if ((x + y) % 3 != 0) setPixel(x, y, true);
            }
        }
        else {
            // Fallback: zeige Temperatur als Text, wenn Icon nicht matcht
            int t = roundf(temp);
            if (abs(t) >= 10) {
                drawDigit(abs(t) / 10, 2, 4);
                drawDigit(abs(t) % 10, 9, 4);
            } else {
                drawDigit(abs(t), 7, 4);
            }
            setPixel(15, 3, true);
        }
    }

    update();
}

// ------------------------------------------------------
// Ende der Datei
// ------------------------------------------------------
