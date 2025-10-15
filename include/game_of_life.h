#pragma once

#include <Arduino.h>
#include "display.h"

class GameOfLife
{
public:
    // Erzeuge GameOfLife, referenziert dein Display
    GameOfLife(Display &display);

    // Initialisierung (z.B. Aufruf in setup)
    void begin(uint16_t stepIntervalMs = 200);

    // Steuerung
    void start(); // Animation starten
    void stop();  // Animation stoppen
    bool isRunning() const;

    // Steuerbefehle
    void randomize(uint8_t fillPercent = 30); // Füllt Zellen zufällig (0-100%)
    void clear();                             // Löscht Feld
    void toggleCell(uint8_t x, uint8_t y);    // Einzeln toggeln
    void setCell(uint8_t x, uint8_t y, bool on);

    // Einstellbar
    void setStepInterval(uint16_t ms);
    uint16_t getStepInterval() const;

    // Nicht-blockierender Aufruf in loop()
    void update();

    // Ein einzelner Simulations-Schritt (kann auch extern aufgerufen werden)
    void step();

    // Zeichnet das aktuelle Feld auf das Display
    void draw();

    // Erstellt ein fliegendes Muster (Rakete / Glider)
    void spawnGlider(uint8_t startX = 5, uint8_t startY = 5);

    // Aktiviert/Deaktiviert automatisches Reset bei Stillstand
    void setAutoReset(bool enabled = true);

private:
    Display &disp;
    uint8_t grid[16][16];
    uint8_t nextGrid[16][16];

    bool running = false;
    uint32_t lastStepMillis = 0;
    uint16_t stepInterval = 200; // ms

    // Helfer
    uint8_t countNeighborsWrapped(int x, int y) const;

    bool autoResetEnabled = true;
    uint8_t stagnantCounter = 0;
    uint8_t stagnantThreshold = 25; // Anzahl Schritte ohne Änderung, bevor Reset
    uint8_t lastGrid[16][16];

    static const uint8_t HISTORY_SIZE = 5;
    uint8_t history[HISTORY_SIZE][16][16];
    uint8_t historyIndex = 0;
};
