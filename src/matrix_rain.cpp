#include "matrix_rain.h"
#include "settings_manager.h"

MatrixRain::MatrixRain(Display &display) : disp(display) { resetColumns(); }

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
    lastRender = millis();
    pwmPhase = 0;
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

    unsigned long now = millis();

    // advance columns
    for (uint8_t x = 0; x < 16; ++x) {
        if (headY[x] < 0 && random(100) < 12) spawnColumn(x);
        if (headY[x] >= 0 && now - lastStep[x] >= speedMs[x]) {
            lastStep[x] = now;
            headY[x]++;
            if (headY[x] - 1 > 15 + trailLength[x]) headY[x] = -1;
        }
    }

    // render at ~60fps with PWM phase cycling for trail dimming
    if (now - lastRender < 16) return; // ~60Hz
    lastRender = now;
    pwmPhase = (pwmPhase + 1) & 0x03; // 0..3

    disp.clear();

    uint8_t globalBrightness = settingsManager.getBrightness();
    // Head = full brightness via display.setPixel
    // Trail = dithered: show only on some pwm phases -> effectively dimmer

    for (uint8_t x = 0; x < 16; ++x) {
        if (headY[x] < 0) continue;
        for (int d = 0; d < trailLength[x]; ++d) {
            int y = headY[x] - d;
            if (y < 0 || y > 15) continue;

            if (d == 0) {
                // head at full brightness
                disp.setPixel(x, y, true);
            } else {
                // trail dimming: farther segments are shown fewer phases
                // steps: d1 ~75%, d2 ~50%, d3+ ~25% of frames
                bool on = false;
                if (d == 1) {
                    on = (pwmPhase != 0);           // 3/4
                } else if (d == 2) {
                    on = (pwmPhase % 2 == 0);       // 2/4
                } else {
                    on = (pwmPhase == 0);           // 1/4
                }

                // add a small texture randomness
                if (((x + y + d + (now >> 6)) & 0x03) == 0) on = !on;

                if (on) disp.setPixel(x, y, true);
            }
        }
    }

    // ensure global brightness applied
    disp.setBrightness(globalBrightness);
    disp.update();
}
