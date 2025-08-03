#include "DeviceProgramManager.h"

EventManager* DeviceProgramManager::eventManager = nullptr;


void DeviceProgramManager::addDeviceProgram(DeviceProgram& deviceProgram)
{
    devicePrograms.push_back(&deviceProgram);
}

/*bool DeviceProgramManager::addProgram(std::unique_ptr<DeviceProgram> program)
{
    for (const auto& p : devicePrograms) {
        if (p->name == program->name) {
            eventManager.debug("Program with name '" + program->name + "' already exists", 0);
            return false;
        }
    }

    devicePrograms.push_back(std::move(program));
    return savePrograms(config);
}*/


DeviceProgram* DeviceProgramManager::getDeviceProgramById(const String& id)
{
    for (auto deviceProgram : devicePrograms) {
        if (deviceProgram->id == id) {
            return deviceProgram;
        }
    }
    return nullptr;
}


DeviceProgram* DeviceProgramManager::getDeviceProgramByName(const String& name)
{
    for (auto deviceProgram : devicePrograms) {
        if (deviceProgram->name == name) {
            return deviceProgram;
        }
    }
    return nullptr;
}


void DeviceProgramManager::removeDeviceProgram(const String& id)
{
    auto deviceProgram = getDeviceProgramById(id);
    if (deviceProgram) {
        devicePrograms.erase(
            std::remove_if(devicePrograms.begin(), devicePrograms.end(), 
                [&](DeviceProgram* dp) { 
                    return dp && dp->id == id; 
                }), 
            devicePrograms.end()
        );
    } else {
        eventManager->debug("Device program with id '" + id + "' not found", 0);
    }
}

const std::vector<DeviceProgram*>& DeviceProgramManager::getAllDevicePrograms()
{
    return devicePrograms;
}

bool DeviceProgramManager::importDeviceProgram(const String& json, String& errorMsg)
{
    DeviceProgram deviceProgram;
    if (deviceProgram.fromJson(json, deviceManager, timeManager, errorMsg)) {
        for (auto existingProgram : devicePrograms) {
            if (existingProgram->id == deviceProgram.id) {
                errorMsg = "Un programme avec l'ID '" + deviceProgram.id + "' existe déjà.";
                return false;
            }
        }

        devicePrograms.push_back(&deviceProgram);
        return true;
    } else {
        errorMsg = "Erreur lors de l'importation du programme : " + errorMsg;
        return false;
    }
}

bool DeviceProgramManager::loadDevicePrograms(bool clearExisting)
{
    if (clearExisting) {
        devicePrograms.clear();
    }

    String json;
    if (!config.loadProgramsJson(json)) {
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err)
        return false;

    if (!doc.is<JsonArray>())
        return false;

    for (JsonObject obj : doc.as<JsonArray>()) {
        String progStr;
        serializeJson(obj, progStr);

        DeviceProgram deviceProgram;
        String error;
        if (deviceProgram.fromJson(progStr, deviceManager, timeManager, error)) {
            //devicePrograms.push_back(deviceProgram);  // Copie dans le vector
            devicePrograms.push_back(&deviceProgram);
        } else {
            eventManager->debug("Erreur chargement programme : " + error, 0);
        }
    }

    return true;
}

bool DeviceProgramManager::saveDevicePrograms()
{
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (auto deviceProgram : devicePrograms) {
        String progJson = deviceProgram->toJson();
        JsonDocument tmp;
        deserializeJson(tmp, progJson);
        arr.add(tmp);
    }

    String output;
    serializeJson(doc, output);
    return config.saveProgramsJson(output);
}


void DeviceProgramManager::clearDevicePrograms()
{
    devicePrograms.clear();
}
