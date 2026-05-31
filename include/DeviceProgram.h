#ifndef DEVICEPROGRAM_H
#define DEVICEPROGRAM_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Managers/DeviceManager.h>
#include <Devices/Device.h>
#include <Managers/TimeManager.h>
#include <Managers/EventManager.h>
#include <functional>
#include <vector>

class Device;
class DeviceManager;

/**
 * @brief DeviceProgram - Represents a program that can control multiple devices
 * Provides:
 * - Device management (add/remove devices)
 * - Program lifecycle management (start/stop)
 * - JSON serialization/deserialization for program settings
 * Based on the TimeManager's Program structure, but extended for device control.
 */
class DeviceProgram
{
  private:
    
    static EventManager* eventManager;  // Pointeur vers EventManager

  public:
    String name;
    String id;
    bool enabled = true;
    Program* program = nullptr;
    std::vector<Device*> devices;
    String settingsJson;

    // Set in fromJson(): needed to unregister the program's time schedulers on destruction.
    TimeManager* timeManager = nullptr;

    DeviceProgram(EventManager& eventMgr);

    ~DeviceProgram()
    {
        if (program) {
            // CRITICAL: remove the time schedulers that capture a pointer to `program`
            // BEFORE freeing it. Otherwise a stale scheduler would fire later and call a
            // method on freed memory (use-after-free) -> crash after days/weeks.
            if (timeManager) {
                timeManager->clearScheduler(program->getStartSchedulerId());
                timeManager->clearScheduler(program->getStopSchedulerId());
            }
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
