#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"   // enthält EEPROM_MAGIC und EEPROM_SIZE

struct Settings {
    uint8_t brightness;     // Helligkeit (10–255)
    uint8_t displayMode;    // Anzeigemodus (0–2)
    bool autoSync;          // Automatische Synchronisation
    uint32_t magic;         // Prüfsignatur zur Validierung
};

class SettingsManager {
public:
    SettingsManager();

    bool begin();       // Initialisiert EEPROM und lädt Einstellungen
    void load();        // Lädt Daten aus EEPROM
    void save();        // Schreibt aktuelle Einstellungen in EEPROM
    void reset();       // Setzt Standardwerte und speichert sie

    // Zugriffsmethoden
    uint8_t getBrightness();
    void setBrightness(uint8_t value);

    uint8_t getDisplayMode();
    void setDisplayMode(uint8_t mode);

    bool getAutoSync();
    void setAutoSync(bool enabled);

private:
    Settings settings;
    bool validate();          // Überprüft Konsistenz und Gültigkeit der Daten
    const int EEPROM_ADDR = 0; // Startadresse des Settings-Blocks im EEPROM
};

extern SettingsManager settingsManager;

#endif
