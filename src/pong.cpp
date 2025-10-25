#include "pong.h"

Pong::Pong()
    : running(false),
      lastFrame(0),
      ballX(8),
      ballY(8),
      velX(0.8f),
      velY(0.6f),
      paddleTopX(6),
      paddleBottomX(6) {}

void Pong::start() {
    running = true;
    ballX = 8;
    ballY = 8;
    velX = 0.8f;
    velY = 0.6f;
    paddleTopX = 6;
    paddleBottomX = 6;
    Serial.println("[Pong] gestartet (endlos)");
}

void Pong::stop() {
    running = false;
    display.clear();
    display.update();
    Serial.println("[Pong] gestoppt");
}

void Pong::update() {
    if (!running) return;

    if (millis() - lastFrame < 60) return; // gleichmäßiges Tempo
    lastFrame = millis();

    // Bewegung
    ballX += velX;
    ballY += velY;

    // Seitenwände
    if (ballX <= 0 || ballX >= 15) velX *= -1;

    // obere Paddle
    if (ballY <= 0) {
        velY *= -1;
        ballY = 1;
    }

    // untere Paddle
    if (ballY >= 15) {
        velY *= -1;
        ballY = 14;
    }

    // einfache KI — paddles folgen Ballposition, aber leicht verzögert
    if (ballY < 8) {
        if (ballX > paddleTopX + 2) paddleTopX++;
        else if (ballX < paddleTopX + 1) paddleTopX--;
    } else {
        if (ballX > paddleBottomX + 2) paddleBottomX++;
        else if (ballX < paddleBottomX + 1) paddleBottomX--;
    }

    paddleTopX = constrain(paddleTopX, 0, 12);
    paddleBottomX = constrain(paddleBottomX, 0, 12);

    // Zeichnen
    display.clear();

    // Ball
    display.setPixel(round(ballX), round(ballY), true);

    // Paddles
    for (int x = paddleTopX; x < paddleTopX + 4; x++) display.setPixel(x, 0, true);
    for (int x = paddleBottomX; x < paddleBottomX + 4; x++) display.setPixel(x, 15, true);

    display.update();
}
