#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <time.h>

class TimeManager {
public:
    TimeManager();
    bool begin();
    bool update();
    
    uint8_t getHour();
    uint8_t getMinute();
    uint8_t getSecond();
    uint8_t getDay();
    uint8_t getMonth();
    uint16_t getYear();
    
    bool isSynced();
    String getTimeString();
    String getDateString();
    
private:
    struct tm timeinfo;
    bool synced;
    unsigned long lastSync;
    
    bool syncTime();
};

extern TimeManager timeManager;

#endif
