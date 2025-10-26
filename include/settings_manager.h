#pragma once

#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"

// EEPROM-Konfiguration
#define EEPROM_MAGIC 0x42AF
#define EEPROM_VERSION 2
#define EEPROM_ADDR 0
#define EEPROM_SIZE 64

// Speichergrenze für Stadtnamen (inkl. Nullterminator)
#ifndef CITY_MAX_LEN
#define CITY_MAX_LEN 32
#endif

// Defaultwerte
constexpr uint8_t DEFAULT_BRIGHTNESS = 100;
constexpr uint8_t DEFAULT_MODE = 0;
constexpr bool DEFAULT_AUTOSYNC = true;

struct Settings {
    uint8_t version;
    uint8_t brightness;
    uint8_t displayMode;
    bool autoSync;
    char city[CITY_MAX_LEN];
    uint16_t magic;
};

class SettingsManager {
public:
    SettingsManager();

    bool begin();
    void load();
    void save();
    void reset();

    // Getter / Setter
    uint8_t getBrightness();
    void setBrightness(uint8_t value);

    uint8_t getDisplayMode();
    void setDisplayMode(uint8_t mode);

    bool getAutoSync();
    void setAutoSync(bool enabled);

    // Wetter: Stadt (Persistenz)
    String getCity();
    void setCity(const String& city);

private:
    Settings settings;
    bool validate();   // nur intern
};

extern SettingsManager settingsManager;
