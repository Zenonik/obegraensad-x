#pragma once

#include <Arduino.h>
#include "display.h"

class MatrixRain {
public:
    explicit MatrixRain(Display &display);

    void start();
    void stop();
    void update();

    bool isRunning() const { return running; }

private:
    Display &disp;
    bool running = false;
    unsigned long lastFrame = 0;

    // Per-column parameters
    int8_t headY[16];        // -1 means inactive
    uint8_t trailLength[16]; // 3..12
    uint16_t speedMs[16];    // 40..140 ms
    unsigned long lastStep[16];

    void resetColumns();
    void spawnColumn(uint8_t x);
};
