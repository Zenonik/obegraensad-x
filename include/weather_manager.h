#ifndef WEATHER_MANAGER_H
#define WEATHER_MANAGER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class WeatherManager {
public:
    void begin(const String& city);
    void update(const bool withoutCheckmark = false);
    void setCity(const String& city);
    String getCity() const;

    float getTemperature() const;
    String getCondition() const;

private:
    String city;
    float temperature = 0.0;
    String condition = "N/A";
    unsigned long lastUpdate = 0;
};

extern WeatherManager weatherManager;

#endif
