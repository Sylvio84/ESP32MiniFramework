#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <Arduino.h>
#include <Configuration.h>
#include <SerialCommandManager.h>
#include <DisplayManager.h>
#include <WiFiManager.h>
#include <MQTTManager.h>
#ifndef DISABLE_ESPUI
#include <ESPUIManager.h>
#endif
#include <EventManager.h>
#include <TimeManager.h>
#include <Tools.h>
#include <Device.h>
#include <LittleFS.h>

#define DEBUG_LOG() debugLog(__FILE__, __LINE__)

inline void debugLog( const char* file, int line)
{
    Serial.printf("(File: %s, Line: %d)\n", file, line);
}

class MainController
{
protected:
    
    EventManager eventManager;
    Configuration& config;
    SerialCommandManager serialCommandManager;
    DisplayManager displayManager;
    WiFiManager wiFiManager;
    MQTTManager mqttManager;
    TimeManager timeManager;

    #ifndef DISABLE_ESPUI
    ESPUIManager espUIManager;
    #endif

    int powerSaving = 0; // 0 = disabled, else = idle time in ms while power saving (100 is a good value)
    bool timeSet = false;

    std::vector<Device*> devices;

    uint powerSavingRemumeTimer = 0;
    void setPowerSaving(int value, bool save = true);

public:
    MainController(Configuration &config);

    virtual void init();
    virtual void loop();

    void addDevice(Device* device);
    std::vector<Device*> getDevices();
    Device* getDeviceById(const String &id) const;
    Device* getDeviceByName(const String &name) const;
    Device* getDeviceByTopic(const String &topic) const;

#ifndef DISABLE_ESPUI
    virtual void processUI(String action, std::vector<String> params);
#endif
    virtual void processCommand(String command, std::vector<String> params);
    virtual void processEvent(String type, String event, std::vector<String> params);

    virtual void processMQTT(String topic, String value);

    EventManager* getEventManager();

    void processDebugMessage(String message, int level = 0, bool displayTime = true);
};

#endif