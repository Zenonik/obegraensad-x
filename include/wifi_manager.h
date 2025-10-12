#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>

class WiFiConnectionManager {
public:
    WiFiConnectionManager();
    bool begin();
    bool isConnected();
    String getSSID();
    String getIP();
    int getRSSI();
    void reset();
    
private:
    WiFiManager wifiManager;
    DNSServer dnsServer;
    bool connected;
    
    static void onConfigMode(WiFiManager *manager);
};

extern WiFiConnectionManager wifiConnection;

#endif
