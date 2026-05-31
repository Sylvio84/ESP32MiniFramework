#include "DeviceProgram.h"

EventManager* DeviceProgram::eventManager = nullptr;

DeviceProgram::DeviceProgram(EventManager& eventMgr)
{
    if (eventManager == nullptr) {
        eventManager = &eventMgr;
    }
}

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

void DeviceProgram::startDevices()
{
    eventManager->debug("Starting devices for program: " + name, 0);
    eventManager->triggerEvent("program", "start", {id, name});
    for (Device* d : devices) {
        if (d) {
#ifdef ESP32
            try {
                d->onProgramStart();
            } catch (const std::exception& e) {
                Serial.println("Error in onProgramStart: " + String(e.what()));
            }
#else
            d->onProgramStart();
#endif
        }
    }
}

void DeviceProgram::stopDevices()
{
    eventManager->debug("Stopping devices for program: " + name, 0);
    eventManager->triggerEvent("program", "stop", {id, name});
    for (Device* d : devices) {
        if (d) {
#ifdef ESP32
            try {
                d->onProgramEnd();
            } catch (const std::exception& e) {
                Serial.println("Error in onProgramEnd: " + String(e.what()));
            }
#else
            // For other platforms, we might not have the same exception handling
            d->onProgramEnd();
#endif
        }
    }
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
    #ifdef ESP8266
        // Limit capacity for ESP8266 to save memory
        doc.shrinkToFit();
    #endif

    doc["name"] = name;
    doc["id"] = id;
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
    String programJson = TimeManager::exportProgramToJson(*program);
    deserializeJson(doc["program"], programJson);  // merge dans l’objet

    String out;
    serializeJson(doc, out);
    return out;
}

bool DeviceProgram::fromJson(const String& json, DeviceManager& deviceManager, TimeManager& timeManager, String& errorMsg)
{
    // Debug: afficher le JSON reçu et sa taille
    eventManager->debug("Importing DeviceProgram from JSON (length=" + String(json.length()) + "): " + json, 2);

    JsonDocument doc;
    #ifdef ESP8266
        // Limit capacity for ESP8266 to save memory
        doc.shrinkToFit();
    #endif
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        errorMsg = "JSON parse error: " + String(error.c_str()) + " - JSON length: " + String(json.length()) + ", First 100 chars: " + json.substring(0, 100);
        eventManager->debug("DeviceProgram JSON parse FAILED: " + String(error.c_str()), 0);
        return false;
    }

    if (!doc["id"].is<int>() && !doc["id"].is<String>()) {
        errorMsg = "Missing or invalid 'id'";
        eventManager->debug("DeviceProgram FAILED: Missing or invalid 'id'", 0);
        return false;
    }

    if (!doc["name"].is<String>()) {
        errorMsg = "Missing or invalid 'name'";
        eventManager->debug("DeviceProgram FAILED: Missing or invalid 'name'", 0);
        return false;
    }

    if (doc["id"].is<int>()) {
        id = String(doc["id"].as<int>());
    } else {
        id = doc["id"].as<String>();
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

            auto device = deviceManager.getDeviceById(id);

            if (device) {
                devices.push_back(device);
            } else {
                errorMsg = "Device with id '" + id + "' not found";
                eventManager->debug("DeviceProgram FAILED: Device '" + id + "' not found", 0);
                return false;
            }
            //eventManager->debug("Device added to program: " + device->id, 2);
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

        for (Device* d : devices) {
            if (d == nullptr) {
                errorMsg = "One of the devices in the program is null";
                eventManager->debug("DeviceProgram FAILED: One of the devices is null", 0);
                return false;
            }
        }

        // Keep a handle on the TimeManager so the destructor can unregister the schedulers
        // that capture a pointer to `program` (prevents use-after-free on program removal).
        this->timeManager = &timeManager;

        program = timeManager.addProgram(programJson, std::bind(&DeviceProgram::startDevices, this), std::bind(&DeviceProgram::stopDevices, this));

        if (!program) {
            errorMsg = "Failed to create program from JSON. Check startTime (format HH:MM or HH:MM:SS) and duration (seconds) fields.";
            eventManager->debug("DeviceProgram FAILED: TimeManager.addProgram failed", 0);
            return false;
        }

        /*program = timeManager.addProgram(
            programJson,
            [this](const String& id) {
                // Vérifier que 'this' est toujours valide
                if (!this || !enabled)
                    return;

                for (Device* d : devices) {
                    if (d != nullptr) {
                        try {
                            d->onProgramStart();
                        } catch (...) {
                            Serial.println("Erreur lors de l'appel à onProgramStart()");
                        }
                    }
                }
            },
            [this](const String& id) {
                if (!this || !enabled)
                    return;

                for (Device* d : devices) {
                    if (d != nullptr) {
                        try {
                            d->onProgramEnd();
                        } catch (...) {
                            Serial.println("Erreur lors de l'appel à onProgramEnd()");
                        }
                    }
                }
            });*/
    } else {
        errorMsg = "Missing or invalid 'program'";
        eventManager->debug("DeviceProgram FAILED: Missing or invalid 'program'", 0);
        return false;
    }

    // Log succès avec détails du programme
    String devicesStr = "";
    for (size_t i = 0; i < devices.size(); i++) {
        if (i > 0) devicesStr += ", ";
        devicesStr += devices[i]->id;
    }
    
    eventManager->debug("DeviceProgram SUCCESS: '" + name + "' (id:" + id + ") -> devices[" + devicesStr + "] at " + program->startTime + " for " + String(program->duration) + "s", 0);

    return true;
}
