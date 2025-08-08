#ifndef FRAMEWORKCONTEXT_H
#define FRAMEWORKCONTEXT_H

#include <Arduino.h>

// Forward declarations
class Configuration;
class EventManager;
class SerialCommandManager;
class TimeManager;
class WiFiManager;
class MQTTManager;
class DeviceManager;
class DeviceProgramManager;
class DisplayManager;
class ESPUIManager;

class FrameworkContext {
public:
    Configuration* config = nullptr;
    EventManager* eventManager = nullptr;
    SerialCommandManager* serialCommandManager = nullptr;
    TimeManager* timeManager = nullptr;
    WiFiManager* wiFiManager = nullptr;
    MQTTManager* mqttManager = nullptr;
    DeviceManager* deviceManager = nullptr;
    DeviceProgramManager* deviceProgramManager = nullptr;
    DisplayManager* displayManager = nullptr;
    ESPUIManager* espUIManager = nullptr;

    // Specific registration methods
    void registerService(Configuration* service) { config = service; }
    void registerService(EventManager* service) { eventManager = service; }
    void registerService(SerialCommandManager* service) { serialCommandManager = service; }
    void registerService(TimeManager* service) { timeManager = service; }
    void registerService(WiFiManager* service) { wiFiManager = service; }
    void registerService(MQTTManager* service) { mqttManager = service; }
    void registerService(DeviceManager* service) { deviceManager = service; }
    void registerService(DeviceProgramManager* service) { deviceProgramManager = service; }
    void registerService(DisplayManager* service) { displayManager = service; }
    void registerService(ESPUIManager* service) { espUIManager = service; }

    // Template method for generic access - only for getService
    template<typename T>
    T* getService();
};

// Template specializations
template<> inline Configuration* FrameworkContext::getService<Configuration>() { return config; }
template<> inline EventManager* FrameworkContext::getService<EventManager>() { return eventManager; }
template<> inline SerialCommandManager* FrameworkContext::getService<SerialCommandManager>() { return serialCommandManager; }
template<> inline TimeManager* FrameworkContext::getService<TimeManager>() { return timeManager; }
template<> inline WiFiManager* FrameworkContext::getService<WiFiManager>() { return wiFiManager; }
template<> inline MQTTManager* FrameworkContext::getService<MQTTManager>() { return mqttManager; }
template<> inline DeviceManager* FrameworkContext::getService<DeviceManager>() { return deviceManager; }
template<> inline DeviceProgramManager* FrameworkContext::getService<DeviceProgramManager>() { return deviceProgramManager; }
template<> inline DisplayManager* FrameworkContext::getService<DisplayManager>() { return displayManager; }
template<> inline ESPUIManager* FrameworkContext::getService<ESPUIManager>() { return espUIManager; }

#endif