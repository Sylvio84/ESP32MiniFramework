#include "Managers/ConfigurationManager.h"
#include <Tools.h>
#include "Managers/CommandManager.h"
#include <FrameworkContext.h>

#ifdef ESP32
ConfigurationManager::ConfigurationManager(FrameworkContext& context) : Manager(context) {}

std::vector<String> ConfigurationManager::getPreferenceKeys()
{
    std::vector<String> keys;

    nvs_iterator_t it = nvs_entry_find("nvs", "config", NVS_TYPE_ANY);
    while (it != NULL) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        keys.push_back(String(info.key));
        it = nvs_entry_next(it);
    }

    return keys;
}
#else
ConfigurationManager::ConfigurationManager(FrameworkContext& context) 
    : Manager(context), json_preferences()
{
    eeprom.begin(EEPROM_PREFERENCES_SIZE);
}
#endif

void ConfigurationManager::init()
{
    debug("ConfigurationManager init...", 1);
    
#ifdef ESP32
    prefs.begin("config", false);
    
    // Initialize default NTP and timezone settings if not present
    if (!prefs.isKey("ntp_server")) {
        prefs.putString("ntp_server", "pool.ntp.org");
        debug("Initialized default NTP server: pool.ntp.org", 2);
    }
    if (!prefs.isKey("timezone")) {
        prefs.putString("timezone", "CET-1CEST,M3.5.0,M10.5.0/3");
        debug("Initialized default timezone: CET-1CEST", 2);
    }
#else
    readJsonPreferences();
    debugJsonPreferences();
#endif

    // Register configuration commands with CommandManager
    registerCommands();

    setInitialized(true);
}


// === Configuration API Implementation (from Configuration.cpp) ===

int ConfigurationManager::getValue(const String key, int defaultValue)
{
    return defaultValue;
}

String ConfigurationManager::getValue(const String key, String defaultValue)
{
    return defaultValue;
}

bool ConfigurationManager::setPreference(const String key, int value)
{
#ifdef ESP32
    prefs.putInt(key.c_str(), value);
    return true;
#else
    return writeVariable(key, value);
#endif
}

bool ConfigurationManager::setPreference(const String key, String value)
{
#ifdef ESP32
    prefs.putString(key.c_str(), value);
    return true;
#else
    return writeVariable(key, value);
#endif
}

int ConfigurationManager::getPreference(const String key, int defaultValue)
{
#ifdef ESP32
    return prefs.getInt(key.c_str(), defaultValue);
#else
    return readVariableInt(key, defaultValue);
#endif
}

String ConfigurationManager::getPreference(const String key, const String &defaultValue)
{
#ifdef ESP32
    return prefs.getString(key.c_str(), defaultValue);
#else
    return readVariableString(key, defaultValue);
#endif
}

bool ConfigurationManager::removePreference(const String key)
{
#ifdef ESP32
    return prefs.remove(key.c_str());
#else
    // For ESP8266, remove from JSON preferences and save
    if (json_preferences[key]) {  // Check if key exists (non-null)
        json_preferences.remove(key);
        writeJsonPreferences();
        return true;
    }
    return false;
#endif
}

String ConfigurationManager::getJsonConfig()
{
    JsonDocument doc;
    auto preferences = getPreferences();
    
    for (const auto& pair : preferences) {
        doc[pair.first] = pair.second;
    }
    
    String result;
    serializeJson(doc, result);
    return result;
}

bool ConfigurationManager::setJsonConfig(const String json)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        debug("Failed to parse JSON configuration: " + String(error.c_str()), 0);
        return false;
    }
    
    for (JsonPair kv : doc.as<JsonObject>()) {
        String key = kv.key().c_str();
        if (kv.value().is<int>()) {
            setPreference(key, kv.value().as<int>());
        } else {
            setPreference(key, kv.value().as<String>());
        }
    }
    
    return true;
}

std::map<String, String> ConfigurationManager::getPreferences()
{
    std::map<String, String> result;
    
#ifdef ESP32
    auto keys = getPreferenceKeys();
    for (const auto& key : keys) {
        // Skip keys that haven't been initialized yet
        if (!prefs.isKey(key.c_str())) {
            continue;
        }
        
        // Try to get as string first, then as int if that fails
        String value = prefs.getString(key.c_str(), "");
        if (value.isEmpty()) {
            int intValue = prefs.getInt(key.c_str(), -999999);
            if (intValue != -999999) {
                value = String(intValue);
            }
        }
        if (!value.isEmpty()) {
            result[key] = value;
        }
    }
#else
    for (JsonPair kv : json_preferences.as<JsonObject>()) {
        result[kv.key().c_str()] = kv.value().as<String>();
    }
#endif
    
    return result;
}

bool ConfigurationManager::saveProgramsJson(const String& json)
{
#ifdef ESP32
    return prefs.putString("programs", json) > 0;
#else
    return writeVariable("programs", json);
#endif
}

bool ConfigurationManager::loadProgramsJson(String& outJson)
{
#ifdef ESP32
    // Check if the key exists before trying to read it to avoid NVS error
    if (prefs.isKey("programs")) {
        outJson = prefs.getString("programs", "[]");
    } else {
        outJson = "[]";
    }
    return !outJson.isEmpty();
#else
    outJson = readVariableString("programs", "[]");
    return !outJson.isEmpty();
#endif
}

String ConfigurationManager::getHostname()
{
    return getPreference("hostname", String(HOSTNAME));
}

void ConfigurationManager::setPowerSaving(int value, bool save)
{
    if (value < 0) {
        value = getPreference("power_saving", 0);
    }
    if (value == 1) {
        value = 100;  // default value
    }
    
    if (value > 0) {
        debug("Power saving enabled: process every " + String(value) + "ms", 1);
    } else {
        if (save) {
            debug("Power saving disabled", 1);
        } else {
            debug("Power saving suspended", 3);
        }
    }
    
    if (save) {
        setPreference("power_saving", value);
    }
}

#ifndef ESP32
bool ConfigurationManager::readJsonPreferences()
{
    #ifdef ESP8266
        // Use shared buffer for ESP8266 to save stack memory
        for (int i = 0; i < EEPROM_PREFERENCES_SIZE; i++) {
            sharedBuffer[i] = eeprom.read(i);
        }
        DeserializationError error = deserializeJson(json_preferences, sharedBuffer);
    #else
        // Original implementation for other platforms
        char buffer[EEPROM_PREFERENCES_SIZE];
        for (int i = 0; i < EEPROM_PREFERENCES_SIZE; i++) {
            buffer[i] = eeprom.read(i);
        }
        DeserializationError error = deserializeJson(json_preferences, buffer);
    #endif
    
    return error == DeserializationError::Ok;
}

bool ConfigurationManager::writeJsonPreferences()
{
    #ifdef ESP8266
        // Use shared buffer for ESP8266 to save stack memory
        serializeJson(json_preferences, sharedBuffer, EEPROM_PREFERENCES_SIZE);
        for (int i = 0; i < EEPROM_PREFERENCES_SIZE; i++) {
            eeprom.write(i, sharedBuffer[i]);
        }
    #else
        // Original implementation for other platforms
        char buffer[EEPROM_PREFERENCES_SIZE];
        serializeJson(json_preferences, buffer, EEPROM_PREFERENCES_SIZE);
        for (int i = 0; i < EEPROM_PREFERENCES_SIZE; i++) {
            eeprom.write(i, buffer[i]);
        }
    #endif
    
    eeprom.commit();
    return true;
}

bool ConfigurationManager::writeVariable(const String key, int value)
{
    json_preferences[key] = value;
    return writeJsonPreferences();
}

bool ConfigurationManager::writeVariable(const String key, String value)
{
    json_preferences[key] = value;
    return writeJsonPreferences();
}

int ConfigurationManager::readVariableInt(const String key, int defaultValue)
{
    if (json_preferences.containsKey(key)) {
        return json_preferences[key].as<int>();
    }
    return defaultValue;
}

String ConfigurationManager::readVariableString(const String key, String defaultValue)
{
    if (json_preferences.containsKey(key)) {
        return json_preferences[key].as<String>();
    }
    return defaultValue;
}

void ConfigurationManager::debugJsonPreferences()
{
    debug("JSON Preferences:", 1);
    for (JsonPair kv : json_preferences.as<JsonObject>()) {
        debug("  " + String(kv.key().c_str()) + " = " + kv.value().as<String>(), 1);
    }
}
#endif

void ConfigurationManager::registerCommands()
{
    auto* cmdMgr = static_cast<CommandManager*>(context->getManager("CommandManager"));
    if (!cmdMgr) return;

    // Debug level command
    cmdMgr->registerCommand(Command(
        "config", "debuglevel", "Get/Set debug verbosity level (0-3)",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                setPreference("debug_level", args[0].toInt());
                return "Debug level set to: " + args[0];
            }
            int debugLevel = getPreference("debug_level", 0);
            return "Debug level: " + String(debugLevel);
        }
    ));

    // Hostname command
    cmdMgr->registerCommand(Command(
        "config", "hostname", "Get/Set device hostname",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                setPreference("hostname", args[0]);
                return "Hostname set to: " + args[0];
            }
            return "Hostname: " + getHostname();
        }
    ));

    // Power saving command
    cmdMgr->registerCommand(Command(
        "config", "power_saving", "Get/Set power saving mode (ms, 0=disabled)",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                int value = args[0].toInt();
                if (value < 0) {
                    return "Invalid value. Use >= 0 (ms), 0 = disabled";
                }
                setPowerSaving(value);
                return "Power saving set to: " + String(value) + "ms";
            }
            int powerSaving = getPreference("power_saving", 0);
            return "Power saving: " + String(powerSaving) + "ms";
        }
    ));

    // Generic config get command
    cmdMgr->registerCommand(Command(
        "config", "get", "Get a configuration value by key",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            if (args.size() < 1) {
                return "Usage: config:get <key>";
            }
            String key = args[0];
            
            // Try to get as string first
            String value = getPreference(key, "");
            if (value != "") {
                return key + " = " + value;
            }
            
            // Try as integer
            int intValue = getPreference(key, -999999);
            if (intValue != -999999) {
                return key + " = " + String(intValue);
            }
            
            return "Key not found: " + key;
        }
    ));

    // Generic config set command
    cmdMgr->registerCommand(Command(
        "config", "set", "Set a configuration value",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            if (args.size() < 2) {
                return "Usage: config:set <key> <value>";
            }
            String key = args[0];
            String value = args[1];
            
            // Join all args after key if value contains spaces
            if (args.size() > 2) {
                for (size_t i = 2; i < args.size(); i++) {
                    value += " " + args[i];
                }
            }
            
            // Try to detect if it's a number
            bool isNumber = true;
            for (char c : value) {
                if (!isdigit(c) && c != '-' && c != '.') {
                    isNumber = false;
                    break;
                }
            }
            
            if (isNumber && value.indexOf('.') == -1) {
                // Integer value
                setPreference(key, value.toInt());
            } else {
                // String value
                setPreference(key, value);
            }
            
            return "OK: " + key + " = " + value;
        }
    ));

    // Generic config clear/remove command
    cmdMgr->registerCommand(Command(
        "config", "clear", "Remove a configuration value",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            if (args.size() < 1) {
                return "Usage: config:clear <key>";
            }
            String key = args[0];
            
            // Check if key exists before removing
            String currentValue = getPreference(key, "");
            if (currentValue == "") {
                // Try as integer
                int intValue = getPreference(key, -999999);
                if (intValue == -999999) {
                    return "Key not found: " + key;
                }
            }
            
            if (removePreference(key)) {
                return "Cleared: " + key;
            } else {
                return "Failed to clear: " + key;
            }
        }
    ));

    // Configuration dump command
    cmdMgr->registerCommand(Command(
        "config", "list", "List all configuration settings",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            String result = "Configuration settings:\n";
            auto vars = getPreferences();
            if (vars.empty()) {
                result += "  (no settings)";
            } else {
                for (const auto& pair : vars) {
                    result += "  " + pair.first + " = " + pair.second + "\n";
                }
            }
            return result;
        }
    ));

    // JSON config command
    cmdMgr->registerCommand(Command(
        "config", "json", "Get/Set configuration as JSON",
        CommandSource::Serial, false,
        [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                if (setJsonConfig(args[0])) {
                    return "Configuration updated from JSON";
                } else {
                    return "Failed to parse JSON configuration";
                }
            }
            return getJsonConfig();
        }
    ));


    // Debug level shortcuts as direct commands
    cmdMgr->registerCommand(Command(
        "", "0", "Set debug level to 0 (silent)",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            setPreference("debug_level", 0);
            return "Debug level set to 0";
        }
    ));
    
    cmdMgr->registerCommand(Command(
        "", "1", "Set debug level to 1 (basic)",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            setPreference("debug_level", 1);
            return "Debug level set to 1";
        }
    ));
    
    cmdMgr->registerCommand(Command(
        "", "2", "Set debug level to 2 (detailed)",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            setPreference("debug_level", 2);
            return "Debug level set to 2";
        }
    ));
    
    cmdMgr->registerCommand(Command(
        "", "3", "Set debug level to 3 (verbose)",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            setPreference("debug_level", 3);
            return "Debug level set to 3";
        }
    ));
}