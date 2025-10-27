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
    void handleGetSettings();
    void handleSaveSettings();
    void handleStatus();
    void handleRestart();
    void handleReset();
    void handleOptions();
    void handlePing();
    void handleNotFound();
    void handleOTAUpdate();
    void setCORSHeaders();
    
    String getHTML();
};

extern WebServerManager webServer;

#endif
