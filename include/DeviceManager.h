#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include <Manager.h>
#include <Devices/Device.h>
#include <FrameworkContext.h>

// Forward declarations
class CommandManager;

/**
 * @brief DeviceManager - Manages devices in the ESP32 Mini Framework
 * Provides:
 * - Device registration and lifecycle management
 * - Device command handling
 * - Event-driven communication for device events
 * @note This manager is designed to handle multiple devices.
 * @note Ensure to call `initDevices()` and `loopDevices()` periodically to manage device states.
 * @note The manager supports device-specific commands and events.
 * @note The manager can be extended to support more complex device operations.
 */
class Device;

class DeviceManager : public Manager
{
  private:
    FrameworkContext* context;



    std::vector<Device*> devices;

  public:
    // New constructor with FrameworkContext
    DeviceManager(FrameworkContext& ctx) : Manager(ctx), context(&ctx) { }
    

    void init() override;

    void addDevice(Device& device);
    Device* getDeviceById(const String& id);
    Device* getDeviceByName(const String &name);
    Device* getDeviceByTopic(const String &topic);
    void removeDevice(const String& id);

    const std::vector<Device*>& getAllDevices();

    void initDevices();
    void loopDevices();
    String getName() const override { return "DeviceManager"; }
    bool onCommand(const String& command, const std::vector<String>& params) override;
    
    void registerCommands();

    void processEventDevices(String type, String event, std::vector<String> params);
    //void processMQTTDevices(String topic, String value);
    //void processCommandDevices(String command, std::vector<String> params);
};

#endif