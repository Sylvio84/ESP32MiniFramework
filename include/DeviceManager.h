#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include <Devices/Device.h>
#include <Configuration.h>
#include <EventManager.h>

class Device;

class DeviceManager
{
  private:

    Configuration& config;

    static EventManager* eventManager;  // Pointeur vers EventManager

    std::vector<Device*> devices;

  public:
    DeviceManager(Configuration& config, EventManager& eventMgr) : config(config)
    {
        if (eventManager == nullptr) {
            eventManager = &eventMgr;
        }
    }

    void addDevice(Device& device);
    Device* getDeviceById(const String& id);
    Device* getDeviceByName(const String &name);
    Device* getDeviceByTopic(const String &topic);
    void removeDevice(const String& id);

    const std::vector<Device*>& getAllDevices();

    void initDevices();
    void loopDevices();
    void processEventDevices(String type, String event, std::vector<String> params);
};

#endif