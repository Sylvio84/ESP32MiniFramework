#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include <Devices/Device.h>
#include <FrameworkContext.h>

class Device;

class DeviceManager
{
  private:

    FrameworkContext* context;

    static EventManager* eventManager;  // Pointeur vers EventManager

    std::vector<Device*> devices;

  public:
    // New constructor with FrameworkContext
    DeviceManager(FrameworkContext& ctx) : context(&ctx)
    {
        if (eventManager == nullptr) {
            eventManager = ctx.getService<EventManager>();
        }
    }
    
    // Legacy constructor for compatibility
    DeviceManager(Configuration& config, EventManager& eventMgr) : context(nullptr)
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
    //void processMQTTDevices(String topic, String value);
    //void processCommandDevices(String command, std::vector<String> params);
};

#endif