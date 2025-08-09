#include "DeviceProgramManager.h"
#include <ConfigurationManager.h>
#include <EventManager.h>
#include <DeviceManager.h>
#include <TimeManager.h>
#include <CommandManager.h>


void DeviceProgramManager::init()
{
    // Register device program commands with CommandManager
    registerCommands();
    
    // Initialize device program manager - load programs if needed
    // This could be extended to auto-load programs on startup
    setInitialized(true);
}

bool DeviceProgramManager::onCommand(const String& command, const std::vector<String>& params)
{
    if (command == "import_program") {
        if (params.size() > 0) {
            debug("Importing program from JSON: " + params[0], 0);
            String errorMsg;
            if (!importDeviceProgram(params[0], errorMsg)) {
                debug("Error importing program: " + errorMsg, 0);
            } else {
                debug("Program imported successfully", 0);
            }
        } else {
            debug("Usage: import_program <json_data>", 0);
        }
        return true;
        
    } else if (command == "remove_program") {
        if (params.size() > 0) {
            debug("Removing program with ID: " + params[0], 0);
            if (removeDeviceProgram(params[0])) {
                debug("Program removed successfully", 1);
            } else {
                debug("Failed to remove program with ID: " + params[0], 0);
            }
        } else {
            debug("Usage: remove_program <program_id>", 0);
        }
        return true;
        
    } else if (command == "display_program") {
        auto programs = getAllDevicePrograms();
        if (programs.empty()) {
            debug("No programs found", 0);
        } else {
            debug("List of programs:", 1);
            for (const auto& program : programs) {
                debug(" #" + program->id + " : " + program->name, 0);
            }
        }
        return true;
    }
    
    return false; // Command not handled
}

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
            debug("Device program with id '" + id + "' removed without saving", 3);
            return true;
        }
    } else {
        debug("Device program with id '" + id + "' not found", 0);
        return false;
    }
}

const std::vector<DeviceProgram*>& DeviceProgramManager::getAllDevicePrograms()
{
    return devicePrograms;
}

bool DeviceProgramManager::importDeviceProgram(const String& json, String& errorMsg)
{
    DeviceProgram* deviceProgram = new DeviceProgram(*context->getEventManager());
    DeviceManager* deviceManager = context ? static_cast<DeviceManager*>(context->getManager("DeviceManager")) : nullptr;
    TimeManager* timeManager = context ? static_cast<TimeManager*>(context->getManager("TimeManager")) : nullptr;
    if (deviceManager && timeManager && deviceProgram->fromJson(json, *deviceManager, *timeManager, errorMsg)) {
        for (auto existingProgram : devicePrograms) {
            if (existingProgram->id == deviceProgram->id) {
                //errorMsg = "Un programme avec l'ID '" + deviceProgram->id + "' existe déjà.";
                //delete deviceProgram;
                //return false;
                removeDeviceProgram(existingProgram->id, false);
                debug("A program with ID '" + deviceProgram->id + "' already exists. Replacing it.", 1);
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
    auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
    if (!configMgr || !configMgr->loadProgramsJson(json)) {
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
        debug("Loading program from JSON: " + progStr, 1);

        DeviceProgram* deviceProgram = new DeviceProgram(*context->getEventManager());
        String error;
        DeviceManager* deviceManager = context ? static_cast<DeviceManager*>(context->getManager("DeviceManager")) : nullptr;
        TimeManager* timeManager = context ? static_cast<TimeManager*>(context->getManager("TimeManager")) : nullptr;
        if (deviceManager && timeManager && deviceProgram->fromJson(progStr, *deviceManager, *timeManager, error)) {
            //devicePrograms.push_back(deviceProgram);  // Copie dans le vector
            devicePrograms.push_back(deviceProgram);
            debug("Program loaded: #" + deviceProgram->id + " : " + deviceProgram->name, 0);
        } else {
            debug("Failed to load program: " + error, 1);
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
    context->getEventManager()->debug("Saving device programs JSON: " + output, 0);
    auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
    return configMgr ? configMgr->saveProgramsJson(output) : false;
}

void DeviceProgramManager::clearDevicePrograms()
{
    devicePrograms.clear();
}

void DeviceProgramManager::registerCommands()
{
    auto* cmdMgr = static_cast<CommandManager*>(context ? context->getManager("CommandManager") : nullptr);
    if (!cmdMgr) return;

    // Program list command
    cmdMgr->registerCommand(Command(
        "program", "list", "List all device programs",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            String result = "Device Programs:\\n";
            auto programs = getAllDevicePrograms();
            if (programs.empty()) {
                result += "  (no programs)";
            } else {
                for (const auto& program : programs) {
                    result += "  " + program->id + ": " + program->name + "\\n";
                }
            }
            return result;
        }
    ));

    // Program info command
    cmdMgr->registerCommand(Command(
        "program", "info", "Show detailed program information",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            if (args.size() == 0) {
                return "Usage: program:info <program_id>";
            }
            
            auto program = getDeviceProgramById(args[0]);
            if (program == nullptr) {
                return "Program not found: " + args[0];
            }
            
            String result = "Program Information:\\n";
            result += "  ID: " + program->id + "\\n";
            result += "  Name: " + program->name + "\\n";
            result += "  Enabled: " + String(program->enabled ? "Yes" : "No");
            
            return result;
        }
    ));

    // Program import command  
    cmdMgr->registerCommand(Command(
        "program", "import", "Import program from JSON",
        CommandSource::Serial, false,
        [this](const std::vector<String>& args) -> String {
            if (args.size() == 0) {
                return "Usage: program:import <json_data>";
            }
            
            String errorMsg;
            if (importDeviceProgram(args[0], errorMsg)) {
                return "Program imported successfully";
            } else {
                return "Failed to import program: " + errorMsg;
            }
        }
    ));

    // Program remove command
    cmdMgr->registerCommand(Command(
        "program", "remove", "Remove a device program",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            if (args.size() == 0) {
                return "Usage: program:remove <program_id>";
            }
            
            if (removeDeviceProgram(args[0])) {
                return "Program " + args[0] + " removed successfully";
            } else {
                return "Failed to remove program: " + args[0];
            }
        }
    ));

    // Program export command
    cmdMgr->registerCommand(Command(
        "program", "export", "Export program as JSON",
        CommandSource::Serial, false,
        [this](const std::vector<String>& args) -> String {
            if (args.size() == 0) {
                return "Usage: program:export <program_id>";
            }
            
            auto program = getDeviceProgramById(args[0]);
            if (program == nullptr) {
                return "Program not found: " + args[0];
            }
            
            return program->toJson();
        }
    ));

    // Program load command
    cmdMgr->registerCommand(Command(
        "program", "load", "Load programs from storage",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            bool clearExisting = (args.size() > 0 && args[0] == "clear");
            if (loadDevicePrograms(clearExisting)) {
                auto programs = getAllDevicePrograms();
                return "Loaded " + String(programs.size()) + " programs successfully";
            } else {
                return "Failed to load programs";
            }
        }
    ));

    // Program save command
    cmdMgr->registerCommand(Command(
        "program", "save", "Save programs to storage",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            if (saveDevicePrograms()) {
                return "Programs saved successfully";
            } else {
                return "Failed to save programs";
            }
        }
    ));


    // Register useful aliases
    cmdMgr->registerAlias("programs", "program:list");
    cmdMgr->registerAlias("prog", "program:info");
}
