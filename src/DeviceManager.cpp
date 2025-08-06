#include <DeviceManager.h>

EventManager* DeviceManager::eventManager = nullptr;

void DeviceManager::addDevice(Device& device)
{
    devices.push_back(&device);
}

Device* DeviceManager::getDeviceById(const String& id)
{
    for (auto& device : devices) {
        if (device->id == id) {
            return device;
        }
    }
    return nullptr;
}

Device* DeviceManager::getDeviceByName(const String& name)
{
    for (auto& device : devices) {
        if (device->name == name) {
            return device;
        }
    }
    return nullptr;
}

Device* DeviceManager::getDeviceByTopic(const String& topic)
{
    for (auto& device : devices) {
        if (device->topic == topic) {
            return device;
        }
    }
    return nullptr;
}

void DeviceManager::removeDevice(const String& id)
{
    auto device = getDeviceById(id);
    if (device) {
        devices.erase(
            std::remove_if(devices.begin(), devices.end(), 
                [&](Device* d) { 
                    return d && d->id == id; 
                }), 
            devices.end()
        );
    } else {
        eventManager->debug("Device with id '" + id + "' not found", 0);
    }
}

const std::vector<Device*>& DeviceManager::getAllDevices()
{
    return devices;
}

void DeviceManager::initDevices()
{
    for (auto device : devices) {
        device->init();
    }
}

void DeviceManager::loopDevices()
{
    for (auto device : devices) {
        device->loop();
    }
}

void DeviceManager::processEventDevices(String type, String event, std::vector<String> params)
{
    for (auto device : devices) {
        device->processEvent(type, event, params);
    }
}

/*void DeviceManager::processMQTTDevices(String topic, String value)
{
    for (auto device : devices) {
        if (device->processMQTT(topic, value)) {
            return; // If one device processes the MQTT message, we stop further processing
        }
    }
}

void DeviceManager::processCommandDevices(String command, std::vector<String> params)
{
    for (auto device : devices) {
        if (device->processCommand(command, params)) {
            return true; // If one device processes the command, we stop further processing
        }
    }
    return false; // No device processed the command
}
*/