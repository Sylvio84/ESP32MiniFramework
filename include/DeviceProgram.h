#ifndef DEVICEPROGRAM_H
#define DEVICEPROGRAM_H

#include <Arduino.h>
#include <vector>
#include <functional>
#include <ArduinoJson.h>
#include <TimeManager.h>
#include <DeviceManager.h>
#include <Devices/Device.h>

class Device;
class DeviceManager;

class DeviceProgram {
  public:
    String name;
    String id;
    bool enabled = true;
    TimeManager::Program program;
    std::vector<Device*> devices;
    String settingsJson;

    DeviceProgram();
    DeviceProgram(const String& name, const TimeManager::Program& program);

    void setSettings(const String& json);
    String getSettings() const;

    void addDevice(Device* device);
    void removeDeviceById(const String& deviceId);

    void activate();
    void deactivate();

    //void applyToDevicesOnStart();
    //void applyToDevicesOnStop();

    String toJson() const;
    bool fromJson(const String& json, DeviceManager& deviceManager, TimeManager& timeManager, String& errorMsg);
};

#endif
