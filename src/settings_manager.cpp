#include "settings_manager.h"
#include "config.h"

SettingsManager settingsManager;

// =========================================
//  Konstruktor mit Defaultwerten
// =========================================
SettingsManager::SettingsManager() {
    settings.version = EEPROM_VERSION;
    settings.brightness = DEFAULT_BRIGHTNESS;
    settings.displayMode = DEFAULT_MODE;
    settings.autoSync = DEFAULT_AUTOSYNC;
    // Standard-Stadt aus Konfiguration übernehmen
    strncpy(settings.city, CITY, CITY_MAX_LEN - 1);
    settings.city[CITY_MAX_LEN - 1] = '\0';
    settings.magic = EEPROM_MAGIC;
}

// =========================================
//  Initialisierung
// =========================================
bool SettingsManager::begin() {
    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("❌ [Settings] EEPROM-Init fehlgeschlagen!");
        return false;
    }

    load();
    return true;
}

// =========================================
//  Laden aus EEPROM
// =========================================
void SettingsManager::load() {
    EEPROM.get(EEPROM_ADDR, settings);

    if (!validate()) {
        Serial.println("⚠️  [Settings] Ungültige Daten – Standardwerte laden.");
        reset();
    } else {
        Serial.println("✅ [Settings] Geladen:");
        Serial.printf("   Version: %d\n", settings.version);
        Serial.printf("   Helligkeit: %d\n", settings.brightness);
        Serial.printf("   Modus: %d\n", settings.displayMode);
        Serial.printf("   AutoSync: %s\n", settings.autoSync ? "Ja" : "Nein");
    }
}

// =========================================
//  Speichern in EEPROM
// =========================================
void SettingsManager::save() {
    settings.magic = EEPROM_MAGIC;
    settings.version = EEPROM_VERSION;
    EEPROM.put(EEPROM_ADDR, settings);
    EEPROM.commit();
    Serial.println("💾 [Settings] Gespeichert");
}

// =========================================
//  Reset auf Defaultwerte
// =========================================
void SettingsManager::reset() {
    settings.version = EEPROM_VERSION;
    settings.brightness = DEFAULT_BRIGHTNESS;
    settings.displayMode = DEFAULT_MODE;
    settings.autoSync = DEFAULT_AUTOSYNC;
    strncpy(settings.city, CITY, CITY_MAX_LEN - 1);
    settings.city[CITY_MAX_LEN - 1] = '\0';
    settings.magic = EEPROM_MAGIC;
    save();
    Serial.println("🔄 [Settings] Zurückgesetzt");
}

// =========================================
//  Validierung der Daten
// =========================================
bool SettingsManager::validate() {
    if (settings.magic != EEPROM_MAGIC) return false;
    if (settings.version != EEPROM_VERSION) return false;
    if (settings.brightness < 10 || settings.brightness > 255) return false;
    if (settings.displayMode > DISPLAYMODES) return false;
    if (settings.city[0] == '\0') return false;
    return true;
}

// =========================================
//  Getter / Setter
// =========================================

uint8_t SettingsManager::getBrightness() { return settings.brightness; }

void SettingsManager::setBrightness(uint8_t value) {
    if (value >= 10 && value <= 255 && value != settings.brightness) {
        settings.brightness = value;
        save();
        Serial.printf("[Settings] Neue Helligkeit: %d\n", value);
    } else {
        if (value < 10 || value > 255)
            Serial.println("⚠️  [Settings] Ungültige Helligkeit (10–255)");
    }
}

uint8_t SettingsManager::getDisplayMode() { return settings.displayMode; }

void SettingsManager::setDisplayMode(uint8_t mode) {
    if (mode <= DISPLAYMODES && mode != settings.displayMode) {
        settings.displayMode = mode;
        save();
        Serial.printf("[Settings] Neuer Modus: %d\n", mode);
    }
}

bool SettingsManager::getAutoSync() { return settings.autoSync; }

void SettingsManager::setAutoSync(bool enabled) {
    if (settings.autoSync != enabled) {
        settings.autoSync = enabled;
        save();
        Serial.printf("[Settings] AutoSync: %s\n", enabled ? "An" : "Aus");
    }
}

// =========================================
//  Stadt (Wetter) Getter / Setter
// =========================================
String SettingsManager::getCity() { return String(settings.city); }

void SettingsManager::setCity(const String& city) {
    String trimmed = city;
    trimmed.trim();
    if (trimmed.length() == 0) {
        Serial.println("⚠️  [Settings] Leerer Stadtname ignoriert");
        return;
    }
    // Nur speichern, wenn sich etwas geändert hat
    if (trimmed.equals(String(settings.city))) {
        return;
    }

    strncpy(settings.city, trimmed.c_str(), CITY_MAX_LEN - 1);
    settings.city[CITY_MAX_LEN - 1] = '\0';
    save();
    Serial.printf("[Settings] Neue Stadt: %s\n", settings.city);
}
