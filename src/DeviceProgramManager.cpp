#include "DeviceProgramManager.h"
#include <Configuration.h>
#include <EventManager.h>
#include <DeviceManager.h>
#include <TimeManager.h>

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

bool DeviceProgramManager::removeDeviceProgram(const String& id, bool saveAfterRemoval)
{
    auto deviceProgram = getDeviceProgramById(id);
    if (deviceProgram) {
        devicePrograms.erase(std::remove_if(devicePrograms.begin(), devicePrograms.end(), [&](DeviceProgram* dp) { return dp && dp->id == id; }),
                             devicePrograms.end());
        if (saveAfterRemoval) {
            return saveDevicePrograms();
        } else {
            eventManager->debug("Device program with id '" + id + "' removed without saving", 3);
            return true;
        }
    } else {
        eventManager->debug("Device program with id '" + id + "' not found", 0);
        return false;
    }
}

const std::vector<DeviceProgram*>& DeviceProgramManager::getAllDevicePrograms()
{
    return devicePrograms;
}

bool DeviceProgramManager::importDeviceProgram(const String& json, String& errorMsg)
{
    DeviceProgram* deviceProgram = new DeviceProgram(*eventManager);
    DeviceManager* deviceManager = context ? context->getService<DeviceManager>() : nullptr;
    TimeManager* timeManager = context ? context->getService<TimeManager>() : nullptr;
    if (deviceManager && timeManager && deviceProgram->fromJson(json, *deviceManager, *timeManager, errorMsg)) {
        for (auto existingProgram : devicePrograms) {
            if (existingProgram->id == deviceProgram->id) {
                //errorMsg = "Un programme avec l'ID '" + deviceProgram->id + "' existe déjà.";
                //delete deviceProgram;
                //return false;
                removeDeviceProgram(existingProgram->id, false);
                eventManager->debug("A program with ID '" + deviceProgram->id + "' already exists. Replacing it.", 1);
            }
        }

        devicePrograms.push_back(deviceProgram);
        return saveDevicePrograms();
    } else {
        if (!deviceManager || !timeManager) {
            errorMsg = "DeviceManager or TimeManager not available";
        } else {
            errorMsg = "Erreur lors de l'importation du programme : " + errorMsg;
        }
        delete deviceProgram;
        return false;
    }
}

bool DeviceProgramManager::loadDevicePrograms(bool clearExisting)
{
    if (clearExisting) {
        for (auto program : devicePrograms) {
            delete program;
        }
        devicePrograms.clear();
    }

    String json;
    Configuration* config = context ? context->getService<Configuration>() : nullptr;
    if (!config || !config->loadProgramsJson(json)) {
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
        eventManager->debug("Loading program from JSON: " + progStr, 1);

        DeviceProgram* deviceProgram = new DeviceProgram(*eventManager);
        String error;
        DeviceManager* deviceManager = context ? context->getService<DeviceManager>() : nullptr;
        TimeManager* timeManager = context ? context->getService<TimeManager>() : nullptr;
        if (deviceManager && timeManager && deviceProgram->fromJson(progStr, *deviceManager, *timeManager, error)) {
            //devicePrograms.push_back(deviceProgram);  // Copie dans le vector
            devicePrograms.push_back(deviceProgram);
            eventManager->debug("Program loaded: #" + deviceProgram->id + " : " + deviceProgram->name, 0);
        } else {
            eventManager->debug("Failed to load program: " + error, 1);
            delete deviceProgram;
            return false;
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
    eventManager->debug("Saving device programs JSON: " + output, 0);
    Configuration* config = context ? context->getService<Configuration>() : nullptr;
    return config ? config->saveProgramsJson(output) : false;
}

void DeviceProgramManager::clearDevicePrograms()
{
    devicePrograms.clear();
}
