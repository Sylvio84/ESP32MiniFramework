#ifndef DEVICEPROGRAM_H
#define DEVICEPROGRAM_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DeviceManager.h>
#include <Devices/Device.h>
#include <TimeManager.h>
#include <EventManager.h>
#include <functional>
#include <vector>

class Device;
class DeviceManager;

class DeviceProgram
{
  private:
    
    static EventManager* eventManager;  // Pointeur vers EventManager

  public:
    String name;
    String id;
    bool enabled = true;
    TimeManager::Program* program;
    std::vector<Device*> devices;
    String settingsJson;

    DeviceProgram(EventManager& eventMgr);
    //DeviceProgram(const String& name, EventManager& eventMgr, const TimeManager::Program& program);

    ~DeviceProgram()
    {
        if (program) {
            delete program;
            program = nullptr;
        }
    }

    void setSettings(const String& json);
    String getSettings() const;

    void addDevice(Device* device);
    void removeDeviceById(const String& deviceId);

    void activate();
    void deactivate();

    void startDevices();
    void stopDevices();

    //void applyToDevicesOnStart();
    //void applyToDevicesOnStop();

    String toJson() const;
    bool fromJson(const String& json, DeviceManager& deviceManager, TimeManager& timeManager, String& errorMsg);
};

#endif
