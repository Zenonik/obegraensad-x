#include "game_of_life.h"

GameOfLife::GameOfLife(Display &display) : disp(display) {
    clear();
}

void GameOfLife::begin(uint16_t stepIntervalMs) {
    stepInterval = stepIntervalMs;
    lastStepMillis = millis();
    running = false;
}

void GameOfLife::start() {
    running = true;
    lastStepMillis = millis();
}

void GameOfLife::stop() {
    running = false;
}

bool GameOfLife::isRunning() const {
    return running;
}

void GameOfLife::setStepInterval(uint16_t ms) {
    stepInterval = ms;
}

uint16_t GameOfLife::getStepInterval() const {
    return stepInterval;
}

void GameOfLife::clear() {
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++)
            grid[y][x] = 0;
}

void GameOfLife::randomize(uint8_t fillPercent) {
    if (fillPercent > 100) fillPercent = 100;
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            grid[y][x] = (random(100) < fillPercent) ? 1 : 0;
        }
    }
}

void GameOfLife::toggleCell(uint8_t x, uint8_t y) {
    if (x < 16 && y < 16) grid[y][x] = !grid[y][x];
}

void GameOfLife::setCell(uint8_t x, uint8_t y, bool on) {
    if (x < 16 && y < 16) grid[y][x] = on ? 1 : 0;
}

uint8_t GameOfLife::countNeighborsWrapped(int x, int y) const {
    int count = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = (x + dx + 16) % 16;
            int ny = (y + dy + 16) % 16;
            count += grid[ny][nx] ? 1 : 0;
        }
    }
    return count;
}

void GameOfLife::step() {
    bool changed = false;

    // Nächsten Zustand berechnen
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            uint8_t n = countNeighborsWrapped(x, y);
            if (grid[y][x]) {
                nextGrid[y][x] = (n == 2 || n == 3) ? 1 : 0;
            } else {
                nextGrid[y][x] = (n == 3) ? 1 : 0;
            }
        }
    }

    // Prüfen, ob sich etwas verändert hat
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if (grid[y][x] != nextGrid[y][x]) changed = true;
            grid[y][x] = nextGrid[y][x];
        }
    }

    // Prüfen auf Wiederholung (Oszillation)
    bool repeating = false;
    for (int h = 0; h < HISTORY_SIZE; h++) {
        bool identical = true;
        for (int y = 0; y < 16 && identical; y++) {
            for (int x = 0; x < 16; x++) {
                if (history[h][y][x] != grid[y][x]) {
                    identical = false;
                    break;
                }
            }
        }
        if (identical) {
            repeating = true;
            break;
        }
    }

    // Aktuelles Muster in den Verlauf speichern
    memcpy(history[historyIndex], grid, sizeof(grid));
    historyIndex = (historyIndex + 1) % HISTORY_SIZE;

    // Wenn nichts passiert oder ein Loop erkannt wurde:
    if (!changed || repeating) {
        stagnantCounter++;
    } else {
        stagnantCounter = 0;
    }

    // Reset bei Stillstand oder Oszillation
    if (autoResetEnabled && stagnantCounter > stagnantThreshold) {
        stagnantCounter = 0;
        randomize(30); // oder spawnGlider() oder leer
    }
}

void GameOfLife::draw() {
    disp.clear();
    // Zeichne Zellen: an=display.setPixel(x,y,true)
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if (grid[y][x])
                disp.setPixel(x, y, true);
        }
    }
    disp.update();
}

void GameOfLife::update() {
    uint32_t now = millis();
    if (running && (now - lastStepMillis >= stepInterval)) {
        lastStepMillis = now;
        step();
        draw();
    }
}

void GameOfLife::spawnGlider(uint8_t startX, uint8_t startY) {
    // Sicherstellen, dass Position gültig ist
    clear();
    // Glider-Muster (die klassische "Rakete")
    //  . O .
    //  . . O
    //  O O O
    setCell((startX + 1) % 16, (startY + 0) % 16, true);
    setCell((startX + 2) % 16, (startY + 1) % 16, true);
    setCell((startX + 0) % 16, (startY + 2) % 16, true);
    setCell((startX + 1) % 16, (startY + 2) % 16, true);
    setCell((startX + 2) % 16, (startY + 2) % 16, true);
    draw();
}

void GameOfLife::setAutoReset(bool enabled) {
    autoResetEnabled = enabled;
}
