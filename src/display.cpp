#include "display.h"
#include "config.h"
#include "font.h"
#include "settings_manager.h" // hinzufügen

Display display;

static const int lut[16][16] = {
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
    {0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23}};

Display::Display() : brightness(200), animationActive(false)
{
    memset(framebuffer, 0, sizeof(framebuffer));
}

void Display::begin()
{
    Serial.println("[Display] Initialisiere Display...");

    pinMode(P_EN, OUTPUT);
    pinMode(P_DI, OUTPUT);
    pinMode(P_CLK, OUTPUT);
    pinMode(P_CLA, OUTPUT);

    digitalWrite(P_EN, LOW);
    digitalWrite(P_CLA, LOW);
    digitalWrite(P_CLK, LOW);
    digitalWrite(P_DI, LOW);

    Serial.println("[Display] Pins konfiguriert");

    // 🔹 Brightness aus gespeicherten Settings übernehmen
    brightness = settingsManager.getBrightness();
    Serial.println("[Display] Übernommene Brightness: " + String(brightness));

    clear();
    update();
    startupAnimation();

    Serial.println("[Display] Initialisierung abgeschlossen");
}

void Display::clear()
{
    memset(framebuffer, 0, sizeof(framebuffer));
}

void Display::setBrightness(uint8_t b)
{
    brightness = b;
}

void Display::setPixel(uint8_t x, uint8_t y, bool state)
{
    if (x < 16 && y < 16)
    {
#ifdef ROTATE_DISPLAY
        // Drehung um 180 Grad
        x = 15 - x;
        y = 15 - y;
#endif
        framebuffer[y][x] = state ? brightness : 0;
    }
}

int Display::mapToHardwareIndex(uint8_t x, uint8_t y)
{
    if (x >= 16 || y >= 16)
        return 0;
    return lut[y][x];
}

void Display::shiftOut()
{
    bool hardwareBuffer[256];
    for (int y = 0; y < 16; y++)
    {
        for (int x = 0; x < 16; x++)
        {
            int hwIndex = mapToHardwareIndex(x, y);
            hardwareBuffer[hwIndex] = framebuffer[y][x] > 0;
        }
    }

    for (int i = 0; i < 256; i++)
    {
        digitalWrite(P_DI, hardwareBuffer[i] ? HIGH : LOW);
        digitalWrite(P_CLK, HIGH);
        delayMicroseconds(4);
        digitalWrite(P_CLK, LOW);
        delayMicroseconds(4);
    }
}

void Display::latch()
{
    delayMicroseconds(5);
    digitalWrite(P_CLA, HIGH);
    delayMicroseconds(5);
    digitalWrite(P_CLA, LOW);
}

void Display::update()
{
    shiftOut();
    latch();
}

void Display::drawDigit(uint8_t digit, uint8_t x, uint8_t y)
{
    if (digit > 9)
        return;
    for (int row = 0; row < 7; row++)
    {
        uint8_t line = pgm_read_byte(&font5x7[digit][row]);
        for (int col = 0; col < 5; col++)
        {
            if (line & (1 << (4 - col)))
            {
                setPixel(x + col, y + row, true);
            }
        }
    }
}

// --- neue Positionierung (nach rechts/oben) ---
void Display::drawTime(uint8_t hour, uint8_t minute)
{
    clear();
    drawDigit(hour / 10, 2, 0);
    drawDigit(hour % 10, 9, 0);
    drawDigit(minute / 10, 2, 9);
    drawDigit(minute % 10, 9, 9);
    update();
}

void Display::drawText(const char *text)
{
    clear();

    uint8_t x = 1; // eins nach rechts verschoben
    uint8_t y = 2; // zwei nach oben verschoben

    for (const char *p = text; *p; p++)
    {
        char c = *p;

        // Unterstützt nur Zahlen und Buchstaben (A-Z, 0-9)
        if (c >= '0' && c <= '9')
        {
            drawDigit(c - '0', x, y);
            x += 6; // Abstand zwischen Ziffern
        }
        else if (c >= 'A' && c <= 'Z')
        {
            // Wenn du später Buchstaben hinzufügen willst, hier einbauen
            x += 6;
        }
        else if (c == ' ')
        {
            x += 4; // kleiner Abstand bei Leerzeichen
        }
    }

    update();
}

// --- Asynchrone Matrix-Pulse Animation ---
void Display::startAsyncAnimation()
{
    animationActive = true;
    animationFrame = millis();
}

void Display::stopAsyncAnimation()
{
    animationActive = false;
    clear();
    update();
}

void Display::startupAnimation()
{
    Serial.println("[Display] Startanimation: Stripe Sweep mit Kreis-Fadeout");

    clear();
    update();

    const int speed = 30; // Delay in ms pro Frame

    // 1️⃣ Horizontaler Sweep (von links nach rechts)
    for (int step = 0; step <= 16; step++)
    {
        clear();
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < step; x++)
            {
                if ((x + y) % 2 == 0)
                    setPixel(x, y, true);
            }
        }
        setBrightness(150);
        update();
        delay(speed);
    }

    // 2️⃣ Diagonale Streifenwelle
    for (int frame = 0; frame < 24; frame++)
    {
        clear();
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                int stripe = (x + y + frame) % 8;
                if (stripe < 3)
                    setPixel(x, y, true);
            }
        }

        int b = 120 + (int)(80 * sin(frame * 0.4f)); // weiches Pulsieren
        setBrightness(b);
        update();
        delay(speed);
    }

    // 3️⃣ Vertikales "Lichtfluten" von unten nach oben
    for (int step = 15; step >= 0; step--)
    {
        clear();
        for (int y = step; y < 16; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                if ((x + y + step) % 3 < 2)
                    setPixel(x, y, true);
            }
        }
        setBrightness(180);
        update();
        delay(speed);
    }

    // 4️⃣ Kurzer Aufblitz ("System Ready")
    for (int i = 0; i < 2; i++)
    {
        setBrightness(255);
        update();
        delay(80);
        setBrightness(100);
        update();
        delay(80);
    }

    // 5️⃣ Kreisförmiges Ausblenden von innen nach außen
    const float cx = 7.5f;
    const float cy = 7.5f;

    // Matrix bleibt kurz an
    delay(200);

    for (float radius = 0; radius <= 12; radius += 1.0f)
    {
        // Nur Pixel innerhalb des aktuellen Radius AUS machen
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                float dx = x - cx;
                float dy = y - cy;
                float dist = sqrt(dx * dx + dy * dy);
                if (dist < radius)
                    setPixel(x, y, false);
            }
        }

        update();
        delay(50);
    }

    // sicherstellen, dass alles aus ist
    clear();
    update();

    Serial.println("[Display] Startanimation abgeschlossen (Kreis-Fadeout)");
}

void Display::handleAsyncAnimation()
{
    if (!animationActive)
        return;

    unsigned long now = millis();
    if (now - animationFrame > 60)
    {
        animationFrame = now;

        clear();
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                // asynchrones "Puls-Rauschen"
                float wave = sin((x * 0.4) + (now / 250.0)) + cos((y * 0.3) + (now / 300.0));
                if (wave + (random(-10, 10) / 10.0) > 0.8)
                {
                    setPixel(x, y, true);
                }
            }
        }
        update();
    }
}

void Display::circleAnimation()
{
    const float cx = 7.5f;
    const float cy = 7.5f;

    for (float radius = 0; radius <= 12; radius += 0.5f)
    {
        clear();
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                float dx = x - cx;
                float dy = y - cy;
                float dist = sqrt(dx * dx + dy * dy);
                if (dist < radius)
                    setPixel(x, y, true);
            }
        }
        update();
        delay(30);
    }

    for (float radius = 12; radius >= 0; radius -= 0.5f)
    {
        clear();
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < 16; x++)
            {
                float dx = x - cx;
                float dy = y - cy;
                float dist = sqrt(dx * dx + dy * dy);
                if (dist < radius)
                    setPixel(x, y, true);
            }
        }
        update();
        delay(30);
    }
}

void Display::lineAnimation()
{
    for (int step = 0; step <= 16; step++)
    {
        clear();
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < step; x++)
            {
                if ((x + y) % 2 == 0)
                    setPixel(x, y, true);
            }
        }
        setBrightness(150);
        update();
        delay(30);
    }

    for (int step = 15; step >= 0; step--)
    {
        clear();
        for (int y = 0; y < 16; y++)
        {
            for (int x = 0; x < step; x++)
            {
                if ((x + y) % 2 == 0)
                    setPixel(x, y, true);
            }
        }
        setBrightness(150);
        update();
        delay(30);
    }
}

void Display::drawWeather(float temp, const String &cond, WeatherMode mode)
{
    clear();

    if (mode == WeatherMode::MODE_TEXT)
    {
        int t = round(temp);

        // Temperatur zentriert in der Mitte
        if (abs(t) >= 10)
        {
            drawDigit(abs(t) / 10, 2, 4);  // Linke Ziffer
            drawDigit(abs(t) % 10, 9, 4); // Rechte Ziffer
        }
        else
        {
            drawDigit(abs(t), 7, 4); // Einzelne Ziffer
        }

        // Grad-Punkt rechts neben der Zahl, zentriert
        setPixel(15, 3, true);
    }
    else if (mode == WeatherMode::MODE_ICON)
    {
        clear();

        // ☁️ Wolke (Outline-Stil deiner Vorlage)
        if (cond.indexOf("Cloud") >= 0)
        {
            const int cloud[][2] = {
                {5, 4}, {6, 4}, {7, 4}, {8, 4}, {9, 4}, {3, 5}, {4, 5}, {10, 5}, {11, 5}, {12, 5}, {1, 6}, {2, 6}, {5, 6}, {6, 6}, {13, 6}, {14, 6}, {0, 7}, {1, 7}, {11, 7}, {12, 7}, {15, 7}, {1, 8}, {2, 8}, {15, 8}, {2, 9}, {3, 9}, {14, 9}, {3, 10}, {4, 10}, {5, 10}, {6, 10}, {7, 10}, {8, 10}, {9, 10}, {10, 10}, {11, 10}, {12, 10}, {13, 10}};
            for (auto &p : cloud)
                setPixel(p[0], p[1], true);
        }

        // 🌧️ Regenwolke
        else if (cond.indexOf("Rain") >= 0)
        {
            const int cloud[][2] = {
                {5, 3}, {6, 3}, {7, 3}, {8, 3}, {9, 3}, {3, 4}, {4, 4}, {10, 4}, {11, 4}, {12, 4}, {1, 5}, {2, 5}, {5, 5}, {6, 5}, {13, 5}, {14, 5}, {0, 6}, {1, 6}, {11, 6}, {12, 6}, {15, 6}, {1, 7}, {2, 7}, {15, 7}, {2, 8}, {3, 8}, {14, 8}, {3, 9}, {4, 9}, {5, 9}, {6, 9}, {7, 9}, {8, 9}, {9, 9}, {10, 9}, {11, 9}, {12, 9}, {13, 9}};
            for (auto &p : cloud)
                setPixel(p[0], p[1], true);

            // Tropfen (leicht versetzt, natürlich verteilt)
            const int drops[][2] = {
                {5, 10}, {7, 10}, {9, 10}, {6, 11}, {8, 11}, {10, 11}, {7, 12}, {9, 12}, {11, 12}};
            for (auto &p : drops)
                setPixel(p[0], p[1], true);
        }

        // ☀️ Sonne (Outline-Stil)
        else if (cond.indexOf("Clear") >= 0)
        {
            const int sun[][2] = {
                {6, 6}, {7, 6}, {8, 6}, {9, 6}, {5, 7}, {6, 7}, {7, 7}, {8, 7}, {9, 7}, {10, 7}, {5, 8}, {6, 8}, {7, 8}, {8, 8}, {9, 8}, {10, 8}, {6, 9}, {7, 9}, {8, 9}, {9, 9},

                {7, 4},
                {8, 4},
                {4, 5},
                {11, 5},
                {3, 7},
                {12, 7},
                {4, 10},
                {11, 10},
                {7, 11},
                {8, 11},
                {2, 7},
                {13, 7},
                {3, 5},
                {12, 5},
                {3, 10},
                {12, 10}};
            for (auto &p : sun)
                setPixel(p[0], p[1], true);
        }

        // 🌫️ Nebel
        else if (cond.indexOf("Fog") >= 0)
        {
            for (int y : {8, 6, 4})
                for (int x = 2; x <= 13; x++)
                    if ((x + y) % 3 != 0)
                        setPixel(x, y, true);
        }

        // ❄️ Schnee
        else if (cond.indexOf("Snow") >= 0)
        {
            const int cloud[][2] = {
                // Wolke
                {5, 3},
                {6, 3},
                {7, 3},
                {8, 3},
                {9, 3},
                {3, 4},
                {4, 4},
                {10, 4},
                {11, 4},
                {12, 4},
                {1, 5},
                {2, 5},
                {5, 5},
                {6, 5},
                {13, 5},
                {14, 5},
                {0, 6},
                {1, 6},
                {11, 6},
                {12, 6},
                {15, 6},
                {1, 7},
                {2, 7},
                {15, 7},
                {2, 8},
                {3, 8},
                {14, 8},
                {3, 9},
                {4, 9},
                {5, 9},
                {6, 9},
                {7, 9},
                {8, 9},
                {9, 9},
                {10, 9},
                {11, 9},
                {12, 9},
                {13, 9}};
            for (auto &p : cloud)
                setPixel(p[0], p[1], true);

            // Schneeflocken
            const int snowflakes[][2] = {
                {5, 11}, {9, 12}, {13, 11} // Drei Schneeflocken, locker verteilt
            };
            for (auto &p : snowflakes)
                setPixel(p[0], p[1], true);
        }

        // ⚡ Gewitter
        else if (cond.indexOf("Thunder") >= 0)
        {
            const int cloud[][2] = {
                // Wolke
                {5, 3},
                {6, 3},
                {7, 3},
                {8, 3},
                {9, 3},
                {3, 4},
                {4, 4},
                {10, 4},
                {11, 4},
                {12, 4},
                {1, 5},
                {2, 5},
                {5, 5},
                {6, 5},
                {13, 5},
                {14, 5},
                {0, 6},
                {1, 6},
                {11, 6},
                {12, 6},
                {15, 6},
                {1, 7},
                {2, 7},
                {15, 7},
                {2, 8},
                {3, 8},
                {14, 8},
                {3, 9},
                {4, 9},
                {5, 9},
                {6, 9},
                {7, 9},
                {8, 9},
                {9, 9},
                {10, 9},
                {11, 9},
                {12, 9},
                {13, 9}};
            for (auto &p : cloud)
                setPixel(p[0], p[1], true);

            // Blitz
            const int lightning[][2] = {
                {7, 10}, // Blitzspitze
                {7, 11},
                {8, 11},
                {8, 12},
                {9, 12},
                {9, 13},
                {10, 13},
                {10, 14} // Blitzfuß
            };
            for (auto &p : lightning)
                setPixel(p[0], p[1], true);
        }
    }

    update();
}
