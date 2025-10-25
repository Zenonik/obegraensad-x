#pragma once
#include "display.h"
#include <Arduino.h>

class Pong {
public:
    Pong();
    void start();
    void stop();
    void update();

    bool isRunning() const { return running; }

private:
    bool running;

    float ballX, ballY;
    float velX, velY;
    int paddleTopX, paddleBottomX;

    unsigned long lastFrame;
};
