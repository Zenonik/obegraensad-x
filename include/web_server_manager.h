#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

class WebServerManager {
public:
    WebServerManager();
    void begin();
    void handleClient();
    
private:
    WebServer server;
    
    void handleRoot();
    void handleLocal();
    void handleDiscover();
    void handleGetSettings();
    void handleSaveSettings();
    void handleStatus();
    void handleRestart();
    void handleReset();
    void handleOptions();
    void handlePing();
    void handleHealth();
    void handleNotFound();
    void handleOTAUpdate();
    void setCORSHeaders();
    
    String getHTML();
    String getDiscoverHTML();
};

extern WebServerManager webServer;

#endif
