#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <Arduino.h>
#include <Configuration.h>
#include <SerialCommandManager.h>
#include <DisplayManager.h>
#include <WiFiManager.h>
#include <MQTTManager.h>
#include <ESPUIManager.h>
#include <EventManager.h>
#include <TimeManager.h>
#include <Tools.h>
#include <Device.h>

#define DEBUG_LOG() debugLog(__FILE__, __LINE__)

inline void debugLog( const char* file, int line)
{
    Serial.printf("(File: %s, Line: %d)\n", file, line);
}

class MainController
{
protected:
    String hostname;
    EventManager eventManager;
    Configuration& config;
    SerialCommandManager serialCommandManager;
    DisplayManager displayManager;
    WiFiManager wiFiManager;
    MQTTManager mqttManager;
    TimeManager timeManager;
    ESPUIManager espUIManager;

    bool timeSet = false;

    std::vector<Device*> devices;

public:
    MainController(Configuration &config);

    virtual void init();
    virtual void loop();

    void addDevice(Device* device);
    std::vector<Device*> getDevices();
    Device* getDeviceById(const String &id) const;
    Device* getDeviceByName(const String &name) const;
    Device* getDeviceByTopic(const String &topic) const;

    virtual void processUI(String action, std::vector<String> params);
    virtual void processCommand(String command, std::vector<String> params);
    virtual void processEvent(String type, String event, std::vector<String> params);

    virtual void processMQTT(String topic, String value);

    EventManager* getEventManager();

    void processDebugMessage(String message, int level = 0);
};

#endif