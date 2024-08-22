#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <Arduino.h>
#include <Configuration.h>
#include <DisplayManager.h>
#include <WiFiManager.h>
#include <MQTTManager.h>
#include <ESPUIManager.h>
#include <EventManager.h>
#include <TimeManager.h>
#include <Tools.h>

class MainController
{
protected:
    const char* hostname;
    Configuration& config;
    DisplayManager displayManager;
    WiFiManager wiFiManager;
    MQTTManager mqttManager;
    EventManager eventManager;
    TimeManager timeManager;
    ESPUIManager espUIManager;

    bool timeSet = false;

public:
    MainController(Configuration &config);

    virtual void init();
    virtual void loop();
    
    void readSerialCommand();

    virtual bool processCommand(String command);
    virtual void processUI(String action, std::vector<String> params);
    virtual void processCommand(String command, std::vector<String> params);
    virtual void processEvent(String type, String action, std::vector<String> params);

    virtual void processMQTT(String topic, String value);

    EventManager* getEventManager();
};

#endif