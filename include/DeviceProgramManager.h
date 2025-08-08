#ifndef DEVICEPROGRAMMANAGER_H
#define DEVICEPROGRAMMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FrameworkContext.h>
#include <Devices/Device.h>
#include <DeviceManager.h>
#include <DeviceProgram.h>
#include <TimeManager.h>
#include <memory>
#include <vector>

class DeviceProgramManager
{
  private:
    FrameworkContext* context;

    static EventManager* eventManager;  // Pointeur vers EventManager

    std::vector<DeviceProgram*> devicePrograms;

  public:
    // New constructor with FrameworkContext
    DeviceProgramManager(FrameworkContext& ctx) : context(&ctx)
    {
        if (eventManager == nullptr) {
            eventManager = ctx.getService<EventManager>();
        }
    }
    
    // Legacy constructor for compatibility
    DeviceProgramManager(Configuration& config, EventManager& eventMgr, DeviceManager& deviceManager, TimeManager& timeManager) : context(nullptr)
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