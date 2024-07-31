#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include <Configuration.h>
#include <WiFi.h>
#include <Tools.h>
#include <Preferences.h>
#include <EventManager.h>

class WiFiManager
{
private:
    static const uint CONNECTION_TIMEOUT = 10000;

    Preferences prefs;
    bool connected = false;
    IPAddress apIP;
    String ApHostname = "ESP32";

    String ssid = "";
    String password = "";

    static EventManager *eventManager; // Pointeur vers EventManager

public:
    WiFiManager(Configuration config, EventManager &eventMgr)
    {
        this->apIP = IPAddress(192, 168, 1, 249);
        this->ApHostname = config.HOSTNAME;
        if (eventManager == nullptr)
        {
            eventManager = &eventMgr;
        }
    }

    void init(bool auto_connect = true);

    bool autoConnect();

    bool connect();

    void disconnect();

    void startAccessPoint();

    //bool processCommand(String command);

    String getSSID();

    bool isConnected();

    void saveSSID(String ssid);

    void savePassword(String password);

    String getDebugInfos();

    String retrieveSSID();

    String retrieveIP();
    
};

#endif