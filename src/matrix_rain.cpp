#include "matrix_rain.h"

MatrixRain::MatrixRain(Display &display) : disp(display) {
    resetColumns();
}

void MatrixRain::resetColumns() {
    for (uint8_t x = 0; x < 16; ++x) {
        headY[x] = -1;
        trailLength[x] = 3 + (random(10)); // 3..12
        speedMs[x] = 60 + (random(80));    // 60..139 ms
        lastStep[x] = 0;
    }
}

void MatrixRain::spawnColumn(uint8_t x) {
    headY[x] = 0;
    trailLength[x] = 3 + (random(10));
    speedMs[x] = 60 + (random(80));
    lastStep[x] = millis();
}

void MatrixRain::start() {
    running = true;
    lastFrame = millis();
    resetColumns();
    disp.clear();
    disp.update();
}

void MatrixRain::stop() {
    running = false;
    disp.clear();
    disp.update();
}

void MatrixRain::update() {
    if (!running) return;

    // Advance columns at their own speed
    unsigned long now = millis();

    // Occasionally spawn a new column on idle ones
    for (uint8_t x = 0; x < 16; ++x) {
        if (headY[x] < 0 && random(100) < 12) {
            spawnColumn(x);
        }
    }

    bool changed = false;
    for (uint8_t x = 0; x < 16; ++x) {
        if (headY[x] >= 0 && now - lastStep[x] >= speedMs[x]) {
            lastStep[x] = now;
            headY[x]++;
            changed = true;
            if (headY[x] - 1 > 15 + trailLength[x]) {
                headY[x] = -1; // deactivate
            }
        }
    }

    if (!changed) return;

    // Draw frame
    disp.clear();

    for (uint8_t x = 0; x < 16; ++x) {
        if (headY[x] >= 0) {
            // Draw head and trail
            for (int d = 0; d < trailLength[x]; ++d) {
                int y = headY[x] - d;
                if (y < 0 || y > 15) continue;
                // Leading head bright: closer to head -> brighter via density
                bool on = true;
                // Sparse some trail pixels for texture
                if (d > 0 && ((x + y + d) % 3 == 0)) on = false;
                if (on) disp.setPixel(x, y, true);
            }
        }
    }

    disp.update();
}
