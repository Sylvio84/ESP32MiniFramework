#ifndef DEVICEPROGRAMMANAGER_H
#define DEVICEPROGRAMMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Configuration.h>
#include <Devices/Device.h>
#include <DeviceManager.h>
#include <DeviceProgram.h>
#include <EventManager.h>
#include <TimeManager.h>
#include <memory>
#include <vector>

class DeviceProgramManager
{
  private:
    Configuration& config;
    DeviceManager& deviceManager;
    TimeManager& timeManager;

    static EventManager* eventManager;  // Pointeur vers EventManager

    std::vector<DeviceProgram*> devicePrograms;

  public:
    DeviceProgramManager(Configuration& config, EventManager& eventMgr, DeviceManager& deviceManager, TimeManager& timeManager) : config(config), deviceManager(deviceManager), timeManager(timeManager)
    {
        if (eventManager == nullptr) {
            eventManager = &eventMgr;
        }
    }

    void addDeviceProgram(DeviceProgram& deviceProgram);
    DeviceProgram* getDeviceProgramById(const String& id);
    DeviceProgram* getDeviceProgramByName(const String& name);
    
    bool removeDeviceProgram(const String& id, bool saveAfterRemoval = true);
    const std::vector<DeviceProgram*>& getAllDevicePrograms();
    bool importDeviceProgram(const String& json, String& errorMsg);

    bool loadDevicePrograms(bool clearExisting = false);
    bool saveDevicePrograms();

    void clearDevicePrograms();
};

#endif  // DEVICEPROGRAMMANAGER_H