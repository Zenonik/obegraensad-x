#include "time_manager.h"
#include "config.h"

TimeManager timeManager;

TimeManager::TimeManager() : synced(false), lastSync(0) {
    memset(&timeinfo, 0, sizeof(timeinfo));
}

bool TimeManager::begin() {
    Serial.println("Initialisiere Zeit-Synchronisation...");
    
    // Zeitzone konfigurieren
    configTime(0, 0, NTP_SERVER);
    setenv("TZ", TIMEZONE, 1);
    tzset();
    
    return syncTime();
}

bool TimeManager::syncTime() {
    Serial.print("Synchronisiere mit NTP-Server...");
    
    int retry = 0;
    while (!getLocalTime(&timeinfo) && retry < 15) {
        Serial.print(".");
        delay(500);
        retry++;
    }
    
    if (retry < 15) {
        synced = true;
        lastSync = millis();
        Serial.println(" OK!");
        Serial.printf("Zeit: %02d:%02d:%02d\n", 
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        return true;
    } else {
        synced = false;
        Serial.println(" FEHLER!");
        return false;
    }
}

bool TimeManager::update() {
    // Re-sync alle 24 Stunden
    if (millis() - lastSync > 86400000) {
        return syncTime();
    }
    
    return getLocalTime(&timeinfo);
}

uint8_t TimeManager::getHour() {
    return timeinfo.tm_hour;
}

uint8_t TimeManager::getMinute() {
    return timeinfo.tm_min;
}

uint8_t TimeManager::getSecond() {
    return timeinfo.tm_sec;
}

uint8_t TimeManager::getDay() {
    return timeinfo.tm_mday;
}

uint8_t TimeManager::getMonth() {
    return timeinfo.tm_mon + 1;
}

uint16_t TimeManager::getYear() {
    return timeinfo.tm_year + 1900;
}

bool TimeManager::isSynced() {
    return synced;
}

String TimeManager::getTimeString() {
    char buffer[9];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", 
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return String(buffer);
}

String TimeManager::getDateString() {
    char buffer[11];
    snprintf(buffer, sizeof(buffer), "%02d.%02d.%04d", 
             timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
    return String(buffer);
}
