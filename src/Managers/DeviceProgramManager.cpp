#include "Managers/DeviceProgramManager.h"
#include "Managers/ConfigurationManager.h"
#include "Managers/EventManager.h"
#include "Managers/DeviceManager.h"
#include "Managers/TimeManager.h"
#include "Managers/CommandManager.h"


void DeviceProgramManager::init()
{
    // Register device program commands with CommandManager
    registerCommands();
    
    // Initialize device program manager - load programs if needed
    // This could be extended to auto-load programs on startup
    setInitialized(true);
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
    bool found = false;
    auto it = devicePrograms.begin();
    while (it != devicePrograms.end()) {
        if (*it && (*it)->id == id) {
            delete *it;  // Libérer la mémoire
            it = devicePrograms.erase(it);  // Retirer de la liste
            found = true;
            break;  // Un seul programme avec cet ID devrait exister
        } else {
            ++it;
        }
    }
    
    if (found) {
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
    DeviceManager* deviceManager = context ? static_cast<DeviceManager*>(context->getManager("DeviceManager")) : nullptr;
    TimeManager* timeManager = context ? static_cast<TimeManager*>(context->getManager("TimeManager")) : nullptr;
    
    if (!deviceManager || !timeManager) {
        errorMsg = "DeviceManager or TimeManager not available";
        return false;
    }
    
    // Parse JSON to detect if it's an array or single object
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        errorMsg = "Invalid JSON format: " + String(error.c_str());
        debug("Import JSON parse FAILED: " + String(error.c_str()), 0);
        return false;
    }
    
    // Handle JSON array (multiple programs)
    if (doc.is<JsonArray>()) {
        debug("Importing ARRAY of " + String(doc.as<JsonArray>().size()) + " programs", 0);
        
        for (JsonObject obj : doc.as<JsonArray>()) {
            String objJson;
            serializeJson(obj, objJson);
            
            DeviceProgram* deviceProgram = new DeviceProgram(*context->getEventManager());
            if (!deviceProgram) {
                errorMsg = "Failed to allocate memory for DeviceProgram";
                return false;
            }
            
            String individualError;
            if (deviceProgram->fromJson(objJson, *deviceManager, *timeManager, individualError)) {
                // Remove existing program with same ID if it exists
                auto it = devicePrograms.begin();
                while (it != devicePrograms.end()) {
                    if (*it && (*it)->id == deviceProgram->id) {
                        debug("Replacing existing program with ID '" + deviceProgram->id + "'", 1);
                        delete *it;
                        it = devicePrograms.erase(it);
                        break;
                    } else {
                        ++it;
                    }
                }
                
                devicePrograms.push_back(deviceProgram);
            } else {
                errorMsg = "Failed to import program: " + individualError;
                delete deviceProgram;
                return false;
            }
        }
        
        return saveDevicePrograms();
    }
    // Handle single JSON object (backward compatibility)
    else if (doc.is<JsonObject>()) {
        debug("Importing SINGLE program object", 0);
        
        DeviceProgram* deviceProgram = new DeviceProgram(*context->getEventManager());
        if (!deviceProgram) {
            errorMsg = "Failed to allocate memory for DeviceProgram";
            return false;
        }
        
        if (deviceProgram->fromJson(json, *deviceManager, *timeManager, errorMsg)) {
            // Remove existing program with same ID if it exists
            auto it = devicePrograms.begin();
            while (it != devicePrograms.end()) {
                if (*it && (*it)->id == deviceProgram->id) {
                    debug("Replacing existing program with ID '" + deviceProgram->id + "'", 1);
                    delete *it;
                    it = devicePrograms.erase(it);
                    break;
                } else {
                    ++it;
                }
            }

            devicePrograms.push_back(deviceProgram);
            return saveDevicePrograms();
        } else {
            errorMsg = "Failed to import program: " + errorMsg;
            delete deviceProgram;
            return false;
        }
    }
    else {
        errorMsg = "JSON must be either an object or an array";
        debug("Import FAILED: JSON is neither object nor array", 0);
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

    // Program list command - Enhanced with more details
    cmdMgr->registerCommand(Command(
        "program", "list", "List all device programs with details",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            String result = "Device Programs:\n";
            auto programs = getAllDevicePrograms();
            if (programs.empty()) {
                result += "  (no programs)\n";
            } else {
                result += "  ID | Name                | Status    | Devices\n";
                result += "  ---|---------------------|-----------|--------\n";
                for (const auto& program : programs) {
                    String status = program->enabled ? "Enabled  " : "Disabled ";
                    String name = program->name;
                    // Pad name to 20 chars
                    while (name.length() < 20) name += " ";
                    if (name.length() > 20) name = name.substring(0, 20);
                    
                    String idStr = program->id;
                    while (idStr.length() < 3) idStr += " ";
                    
                    result += "  " + idStr + "| " + name + "| " + status + " | ";
                    result += String(program->devices.size()) + " device(s)\n";
                }
            }
            return result;
        }
    ));

    // Program info command - Enhanced with full details
    cmdMgr->registerCommand(Command(
        "program", "info", "Show detailed program information",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            if (args.size() == 0) {
                return "Usage: program:info <program_id>";
            }
            
            auto program = getDeviceProgramById(args[0]);
            if (program == nullptr) {
                return "Program not found: " + args[0];
            }
            
            String result = "Program Information:\n";
            result += "  ID: " + program->id + "\n";
            result += "  Name: " + program->name + "\n";
            result += "  Status: " + String(program->enabled ? "Enabled" : "Disabled") + "\n";
            
            // Device list
            result += "  Devices (" + String(program->devices.size()) + "):\n";
            if (program->devices.empty()) {
                result += "    (no devices)\n";
            } else {
                for (const auto& device : program->devices) {
                    if (device) {
                        result += "    - " + device->id + " (" + device->name + ")\n";
                    }
                }
            }
            
            // Program timing details
            if (program->program) {
                result += "  Schedule:\n";
                if (!program->program->startTime.isEmpty()) {
                    result += "    Start Time: " + program->program->startTime + "\n";
                }
                if (program->program->duration > 0) {
                    result += "    Duration: " + String(program->program->duration) + " minute(s)\n";
                }
                if (!program->program->startDate.isEmpty()) {
                    result += "    Start Date: " + program->program->startDate + "\n";
                }
                if (!program->program->endDate.isEmpty()) {
                    result += "    End Date: " + program->program->endDate + "\n";
                }
                if (!program->program->daysOfWeek.empty()) {
                    result += "    Days: ";
                    String days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
                    bool first = true;
                    for (int day : program->program->daysOfWeek) {
                        if (day >= 0 && day < 7) {
                            if (!first) result += ", ";
                            result += days[day];
                            first = false;
                        }
                    }
                    result += "\n";
                }
                result += "    Active: " + String(program->program->active ? "Yes" : "No") + "\n";
            }
            
            // Settings if present
            if (!program->settingsJson.isEmpty()) {
                result += "  Settings: " + program->settingsJson + "\n";
            }
            
            return result;
        }
    ));

    // Program import command  
    cmdMgr->registerCommand(Command(
        "program", "import", "Import program from JSON",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            
            //Serial.println("program:import called");
            //debug("program:import called", 0);
            
            if (args.size() == 0) {
                return "Usage: program:import <json_data>";
            }
            
            // Concatener tous les arguments au cas où le JSON a été splitté par des espaces
            String jsonData = args[0];
            for (size_t i = 1; i < args.size(); i++) {
                jsonData += " " + args[i];
            }
            
            // Debug pour voir ce qui est reçu
            debug("program:import received " + String(args.size()) + " args, total JSON length: " + String(jsonData.length()), 0);
            
            String errorMsg;
            if (importDeviceProgram(jsonData, errorMsg)) {
                debug("Program import SUCCESS", 0);
                
                // Afficher récapitulatif de tous les programmes actifs
                const auto& programs = getAllDevicePrograms();
                debug("=== ACTIVE PROGRAMS SUMMARY ===", 0);
                for (const auto* prog : programs) {
                    if (prog && prog->enabled && prog->program) {
                        String devicesStr = "";
                        for (size_t i = 0; i < prog->devices.size(); i++) {
                            if (i > 0) devicesStr += ",";
                            devicesStr += prog->devices[i]->id;
                        }
                        debug("ACTIVE: " + prog->name + " [" + devicesStr + "] at " + prog->program->startTime + " for " + String(prog->program->duration) + "s", 0);
                    }
                }
                debug("=== END SUMMARY ===", 0);
                
                return "Program imported successfully";
            } else {
                debug("Program import FAILED: " + errorMsg, 0);
                return "Failed to import program: " + errorMsg;
            }
        }
    ));

    // Program enable command
    cmdMgr->registerCommand(Command(
        "program", "enable", "Enable a device program",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            if (args.size() == 0) {
                return "Usage: program:enable <program_id>";
            }
            
            auto program = getDeviceProgramById(args[0]);
            if (program == nullptr) {
                return "Program not found: " + args[0];
            }
            
            if (program->enabled) {
                return "Program " + args[0] + " (" + program->name + ") is already enabled";
            }
            
            program->activate();
            if (saveDevicePrograms()) {
                return "Program " + args[0] + " (" + program->name + ") enabled successfully";
            } else {
                return "Program enabled but failed to save configuration";
            }
        }
    ));

    // Program disable command
    cmdMgr->registerCommand(Command(
        "program", "disable", "Disable a device program",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            if (args.size() == 0) {
                return "Usage: program:disable <program_id>";
            }
            
            auto program = getDeviceProgramById(args[0]);
            if (program == nullptr) {
                return "Program not found: " + args[0];
            }
            
            if (!program->enabled) {
                return "Program " + args[0] + " (" + program->name + ") is already disabled";
            }
            
            // Stop devices if program is currently running
            program->stopDevices();
            program->deactivate();
            
            if (saveDevicePrograms()) {
                return "Program " + args[0] + " (" + program->name + ") disabled successfully";
            } else {
                return "Program disabled but failed to save configuration";
            }
        }
    ));

    // Program remove command - Enhanced to stop program before removal
    cmdMgr->registerCommand(Command(
        "program", "remove", "Remove a device program",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            if (args.size() == 0) {
                return "Usage: program:remove <program_id>";
            }
            
            auto program = getDeviceProgramById(args[0]);
            if (program == nullptr) {
                return "Program not found: " + args[0];
            }
            
            // Stop devices before removal
            String programName = program->name;
            program->stopDevices();
            
            if (removeDeviceProgram(args[0])) {
                return "Program " + args[0] + " (" + programName + ") removed successfully";
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
        CommandSource::Any, true,
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
        CommandSource::Any, true,
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
    cmdMgr->registerAlias("progenable", "program:enable");
    cmdMgr->registerAlias("progdisable", "program:disable");
    cmdMgr->registerAlias("progdel", "program:remove");
}
