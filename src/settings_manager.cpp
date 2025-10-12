#include "settings_manager.h"
#include "config.h"

SettingsManager settingsManager;

SettingsManager::SettingsManager() {
    settings.brightness = 200;
    settings.displayMode = 0;
    settings.autoSync = true;
    settings.magic = EEPROM_MAGIC;
}

bool SettingsManager::begin() {
    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("❌ EEPROM-Initialisierung fehlgeschlagen!");
        return false;
    }

    load();
    return true;
}

void SettingsManager::load() {
    EEPROM.get(EEPROM_ADDR, settings);

    if (!validate()) {
        Serial.println("⚠️  Ungültige oder leere Einstellungen – Standardwerte werden gesetzt.");
        reset();
    } else {
        Serial.println("✅ Einstellungen geladen:");
        Serial.println("  Helligkeit: " + String(settings.brightness));
        Serial.println("  Modus: " + String(settings.displayMode));
        Serial.println("  Auto-Sync: " + String(settings.autoSync ? "Ja" : "Nein"));
    }
}

void SettingsManager::save() {
    settings.magic = EEPROM_MAGIC;
    EEPROM.put(EEPROM_ADDR, settings);
    EEPROM.commit();
    delay(10);
    Serial.println("💾 Einstellungen gespeichert");
}

void SettingsManager::reset() {
    settings.brightness = 200;
    settings.displayMode = 0;
    settings.autoSync = true;
    settings.magic = EEPROM_MAGIC;
    save();
    Serial.println("🔄 Einstellungen auf Standard zurückgesetzt");
}

bool SettingsManager::validate() {
    if (settings.magic != EEPROM_MAGIC) return false;
    if (settings.brightness < 10 || settings.brightness > 255) return false;
    if (settings.displayMode > 3) return false;
    return true;
}

// -------------------- Getter/Setter --------------------

uint8_t SettingsManager::getBrightness() { return settings.brightness; }

void SettingsManager::setBrightness(uint8_t value) {
    if (value >= 10 && value <= 255) {
        settings.brightness = value;
        save(); // automatisch speichern
    } else {
        Serial.println("⚠️  Ungültige Helligkeit (10–255 erlaubt)");
    }
}

uint8_t SettingsManager::getDisplayMode() { return settings.displayMode; }

void SettingsManager::setDisplayMode(uint8_t mode) {
    if (mode <= 5) {
        settings.displayMode = mode;
        save();
    }
}

bool SettingsManager::getAutoSync() { return settings.autoSync; }

void SettingsManager::setAutoSync(bool enabled) {
    settings.autoSync = enabled;
    save();
}
