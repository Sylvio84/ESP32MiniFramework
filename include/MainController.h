#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <Arduino.h>
#ifdef ESP32
#include <esp_chip_info.h>
#endif

#include <Configuration.h>
#include <SerialCommandManager.h>
#ifndef DISABLE_DISPLAY
#include <DisplayManager.h>
#endif
#include <WiFiManager.h>
#include <MQTTManager.h>
#ifndef DISABLE_ESPUI
#include <ESPUIManager.h>
#endif
#include <EventManager.h>
#include <TimeManager.h>
#include <DeviceProgram.h>
#include <Tools.h>
//#include <Devices/Device.h>
#include <LittleFS.h>
#include <DeviceManager.h>
#include <DeviceProgramManager.h>

#define DEBUG_LOG() debugLog(__FILE__, __LINE__)

#define RELEASE_VERSION "1.1.0"
#define RELEASE_DATE "2025-08-06"

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
    #ifndef DISABLE_DISPLAY
    DisplayManager displayManager;
    #endif
    WiFiManager wiFiManager;
    MQTTManager mqttManager;
    TimeManager timeManager;

    #ifndef DISABLE_ESPUI
    ESPUIManager espUIManager;
    #endif

    int powerSaving = 0; // 0 = disabled, else = idle time in ms while power saving (100 is a good value)
    bool timeSet = false;

    DeviceManager deviceManager;
    DeviceProgramManager deviceProgramManager;

    uint powerSavingRemumeTimer = 0;
    void setPowerSaving(int value, bool save = true);

public:
    MainController(Configuration &config);

    virtual void init();
    virtual void loop();

    void internalLed(bool state);
    bool internalLedState();

#ifndef DISABLE_ESPUI
    virtual void processUI(String action, std::vector<String> params);
#endif
    bool processInput(const String input);
    virtual void processCommand(String command, std::vector<String> params);
    virtual void processEvent(String type, String event, std::vector<String> params);

    virtual void processMQTT(String topic, String value);

    EventManager* getEventManager();

    void processDebugMessage(String message, int level = 0, bool displayTime = true);
};

#endif
