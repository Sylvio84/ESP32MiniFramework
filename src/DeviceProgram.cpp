#include "DeviceProgram.h"

DeviceProgram::DeviceProgram() {}

DeviceProgram::DeviceProgram(const String& name, const TimeManager::Program& program) : name(name), program(program) {}

void DeviceProgram::setSettings(const String& json)
{
    settingsJson = json;
}

String DeviceProgram::getSettings() const
{
    return settingsJson;
}

void DeviceProgram::addDevice(Device* device)
{
    devices.push_back(device);
}

void DeviceProgram::removeDeviceById(const String& deviceId)
{
    devices.erase(std::remove_if(devices.begin(), devices.end(), [&](Device* d) { return d && d->id == deviceId; }), devices.end());
}

void DeviceProgram::activate()
{
    enabled = true;
}

void DeviceProgram::deactivate()
{
    enabled = false;
}

/*
void DeviceProgram::applyToDevicesOnStart() {
    if (!enabled) return;
    for (auto& device : devices) {
        if (device.onStart) device.onStart();
    }
}

void DeviceProgram::applyToDevicesOnStop() {
    if (!enabled) return;
    for (auto& device : devices) {
        if (device.onStop) device.onStop();
    }
}
*/

String DeviceProgram::toJson() const
{
    JsonDocument doc;

    doc["name"] = name;
    doc["enabled"] = enabled;

    JsonArray deviceArray = doc["devices"].to<JsonArray>();
    for (const auto& d : devices) {
        if (d) {
            deviceArray.add(d->id);
        }
    }

    // Settings (stocké comme JSON string)
    if (!settingsJson.isEmpty()) {
        JsonDocument settingsDoc;
        if (deserializeJson(settingsDoc, settingsJson) == DeserializationError::Ok) {
            doc["settings"] = settingsDoc.as<JsonObject>();
        }
    }

    // Program
    String programJson = TimeManager::exportProgramToJson(program);
    deserializeJson(doc["program"], programJson);  // merge dans l’objet

    String out;
    serializeJson(doc, out);
    return out;
}

bool DeviceProgram::fromJson(const String& json, const std::vector<Device*>& availableDevices, TimeManager& timeManager, String& errorMsg)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        errorMsg = "JSON parse error: " + String(error.c_str());
        return false;
    }

    if (!doc["name"].is<String>()) {
        errorMsg = "Missing or invalid 'name'";
        return false;
    }

    name = doc["name"].as<String>();
    enabled = doc["enabled"] | true;

    // Devices : lier aux objets existants
    devices.clear();
    if (doc["devices"].is<JsonArray>()) {
        for (JsonVariant v : doc["devices"].as<JsonArray>()) {
            if (!v.is<String>())
                continue;
            String id = v.as<String>();
            bool found = false;

            for (Device* d : availableDevices) {
                if (d && d->id == id) {
                    devices.push_back(d);
                    found = true;
                    break;
                }
            }

            if (!found) {
                errorMsg = "Device not found: " + id;
                return false;
            }
        }
    }

    // Settings : re-sérialiser pour stocker en tant que String
    if (doc["settings"].is<JsonObject>()) {
        JsonDocument settingsDoc;
        settingsDoc.set(doc["settings"]);
        String s;
        serializeJson(settingsDoc, s);
        setSettings(s);
    }

    // Program
    if (doc["program"].is<JsonObject>()) {
        String programJson;
        serializeJson(doc["program"], programJson);

        program = timeManager.addProgram(
            programJson,
            [this](const String&) {
                for (Device* d : devices) {
                    d->onProgramStart();
                }
            },
            [this](const String&) {
                for (Device* d : devices) {
                    d->onProgramEnd();
                }
            });
    } else {
        errorMsg = "Missing or invalid 'program'";
        return false;
    }

    return true;
}
