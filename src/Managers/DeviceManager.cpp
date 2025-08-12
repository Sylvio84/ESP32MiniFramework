#include "Managers/DeviceManager.h"
#include "Managers/ConfigurationManager.h"
#include "Managers/EventManager.h"
#include "Managers/CommandManager.h"


void DeviceManager::init()
{
    // Register device commands with CommandManager
    registerCommands();
    
    // Initialize devices by calling initDevices
    initDevices();
    setInitialized(true);
}

bool DeviceManager::onCommand(const String& command, const std::vector<String>& params)
{
    if (command == "device") {
        if (params.size() == 0) {
            debug("List of devices:", 0);
            for (const auto& device : getAllDevices()) {
                debug(" #" + device->id + " : " + device->name + " (" + device->topic + ")", 0);
            }
        } else {
            auto device = getDeviceById(params[0]);
            if (device != nullptr) {
                debug("ID: " + device->id, 0);
                debug("Type: " + device->type, 0);
                debug("Name: " + device->name, 0);
                debug("Topic: " + device->topic, 0);
            } else {
                debug("Device not found: " + params[0], 0);
            }
        }
        return true;
    }
    return false; // Command not handled
}

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
        debug("Device with id '" + id + "' not found", 0);
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

void DeviceManager::registerCommands()
{
    auto* cmdMgr = static_cast<CommandManager*>(context ? context->getManager("CommandManager") : nullptr);
    if (!cmdMgr) return;

    // Device list command
    cmdMgr->registerCommand(Command(
        "device", "list", "List all registered devices",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            String result = "Registered devices:\n";
            auto devices = getAllDevices();
            if (devices.empty()) {
                result += "  (no devices)";
            } else {
                for (const auto& device : devices) {
                    result += "  " + device->id + ": " + device->name;
                    result += " (" + device->type + ")";
                    if (!device->topic.isEmpty()) {
                        result += " [topic: " + device->topic + "]";
                    }
                    result += "\n";
                }
            }
            return result;
        }
    ));

    // Device info command
    cmdMgr->registerCommand(Command(
        "device", "info", "Show detailed device information",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            if (args.size() == 0) {
                return "Usage: device:info <device_id>";
            }
            
            auto device = getDeviceById(args[0]);
            if (device == nullptr) {
                return "Device not found: " + args[0];
            }
            
            String result = "Device Information:\n";
            result += "  ID: " + device->id + "\n";
            result += "  Name: " + device->name + "\n";
            result += "  Type: " + device->type + "\n";
            result += "  Topic: " + device->topic + "\n";
            result += "  State: " + String(device->state ? "ON" : "OFF");
            
            return result;
        }
    ));

    // Device command execution
    cmdMgr->registerCommand(Command(
        "device", "cmd", "Send command to device",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            if (args.size() < 2) {
                return "Usage: device:cmd <device_id> <command>";
            }
            
            auto device = getDeviceById(args[0]);
            if (device == nullptr) {
                return "Device not found: " + args[0];
            }
            
            // Create params vector (skip first two args: device_id and command)
            std::vector<String> params;
            for (size_t i = 2; i < args.size(); i++) {
                params.push_back(args[i]);
            }
            
            if (device->processCommand(args[1], params)) {
                return "Command sent to device " + device->id;
            } else {
                return "Command not handled by device " + device->id;
            }
        }
    ));


    // Register useful aliases
    cmdMgr->registerAlias("devices", "device:list");
    cmdMgr->registerAlias("dev", "device:info");
}