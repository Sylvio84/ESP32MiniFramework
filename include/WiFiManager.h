#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include <Configuration.h>
#ifndef DISABLE_ESPUI
#include <ESPUI.h>
#endif
#include <EventManager.h>
#include <Tools.h>

#ifdef ESP32
#include <WiFi.h>
#include <esp_https_ota.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include "ESP8266httpUpdate.h"
#endif

class WiFiManager
{
  private:
    static const uint CONNECTION_TIMEOUT = 10000;

    Configuration& config;

    bool connected = false;
    bool keepConnected = false;
    // 0 = start, 1 = connection in progress, 2 = initial connection failed, 3 = connection lost, 4 = disconnection, 5 = Access Point, 10 = connection success
    uint connectionStatus = 0;
    uint timeout = 0;
    uint checkDelay = 100;
    uint tryCount = 0;

    IPAddress apIP;

    static EventManager* eventManager;  // Pointeur vers EventManager

#ifndef DISABLE_ESPUI
    // ESPUI:
    uint16_t ssidInput = 0;
    uint16_t passwordInput = 0;
#endif

  public:
    WiFiManager(Configuration& config, EventManager& eventMgr) : config(config)
    {
        this->apIP = IPAddress(192, 168, 1, 249);
        if (eventManager == nullptr) {
            eventManager = &eventMgr;
        }
    }

    String ssid = "";
    String password = "";

    void init(bool auto_connect = true);
    void loop();

    void processEvent(String type, String event, std::vector<String> params);
    bool processCommand(String action, std::vector<String> params);
    bool autoConnect();
    bool connect();
    bool keepConnection();
    void disconnect();
    void startAccessPoint();
    String getStatus();
    String getSSID();
    bool isConnected();
    void saveSSID(String ssid);
    void savePassword(String password);
    String getDebugInfos();
    String retrieveSSID();
    String retrievePassword();
    String retrieveIP();
    String getInfo(String name);
    void scanNetworks(bool show_hidden = false);
    int getNetworks();
    int getNetworkCount(bool show_hidden = false);
    void setNetwork(int n, bool save = false);
    String getNetworkInfo(int n, String name);

#ifndef DISABLE_ESPUI
    void initEspUI();
    void EspUiCallback(Control* sender, int type);
#endif

    bool otaUpdate();
};

#endif
