#ifndef DEVICEPROGRAMMANAGER_H
#define DEVICEPROGRAMMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Manager.h"
#include <FrameworkContext.h>
#include <Devices/Device.h>
#include "DeviceManager.h"
#include <DeviceProgram.h>
#include "TimeManager.h"
#include <memory>
#include <vector>
#include <ctime>

// Forward declarations
class CommandManager;

/**
 * @brief DeviceProgramManager - Manages device programs in the ESP32 Mini Framework
 * Provides:
 * - Device program registration and lifecycle management
 * - Device program command handling
 * - Event-driven communication for device program events
 * @note This manager is designed to handle multiple device programs.
 * @note Ensure to call `initDevicePrograms()` and `loopDevicePrograms()` periodically to manage device program states.
 * @note The manager supports device program-specific commands and events.
 * @note The manager can be extended to support more complex device program operations.
 */
class DeviceProgramManager : public Manager
{
  private:
    FrameworkContext* context;


    std::vector<DeviceProgram*> devicePrograms;

  public:
    // New constructor with FrameworkContext
    DeviceProgramManager(FrameworkContext& ctx) : Manager(ctx), context(&ctx) { }
    

    void init() override;

    void addDeviceProgram(DeviceProgram& deviceProgram);
    DeviceProgram* getDeviceProgramById(const String& id);
    DeviceProgram* getDeviceProgramByName(const String& name);
    DeviceProgram* getUpcomingDeviceProgram();
    
    bool removeDeviceProgram(const String& id, bool saveAfterRemoval = true);
    const std::vector<DeviceProgram*>& getAllDevicePrograms();
    bool importDeviceProgram(const String& json, String& errorMsg);

    bool loadDevicePrograms(bool clearExisting = false);
    bool saveDevicePrograms();

    String getName() const override { return "DeviceProgramManager"; }
    
    void registerCommands();

    void clearDevicePrograms();
};

#endif  // DEVICEPROGRAMMANAGER_H