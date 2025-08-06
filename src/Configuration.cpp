#include <Configuration.h>

EventManager* Configuration::eventManager = nullptr;

#ifdef ESP32
Configuration::Configuration() {}

std::vector<String> Configuration::getPreferenceKeys()
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
Configuration::Configuration() : json_preferences()
{
    eeprom.begin(EEPROM_PREFERENCES_SIZE);
}
#endif

void Configuration::init(EventManager& eventMgr)
{
    eventManager = &eventMgr;
#ifdef ESP32
    prefs.begin("config", false);
#else
    readJsonPreferences();
    Serial.println("DEBUG...");
    debugJsonPreferences();
#endif
}

int Configuration::getValue(const String key, int defaultValue)
{
    return defaultValue;
}

String Configuration::getValue(const String key, String defaultValue)
{
    return defaultValue;
}

bool Configuration::setPreference(const String key, int value)
{
    if (key.length() > 16 || key.length() == 0) {
        eventManager->debug("Key too long for NVS", 0);
        return false;
    }

#ifdef ESP32
    prefs.putInt(key.c_str(), value);
    return true;
#else
    return writeVariable(key, value);
#endif
}

bool Configuration::setPreference(const String key, String value)
{
    if (key.length() > 16 || key.length() == 0 || value.length() > 1984) {
        eventManager->debug("Value too long for NVS", 0);
        return false;
    }
#ifdef ESP32
    prefs.putString(key.c_str(), value.c_str());
    return true;
#else
    return writeVariable(key, value);
#endif
}

int Configuration::getPreference(const String key, int defaultValue)
{
#ifdef ESP32
    if (!prefs.isKey(key.c_str())) {
        return defaultValue;
    }
    return prefs.getInt(key.c_str(), defaultValue);
#else
    return readVariableInt(key, defaultValue);
#endif
}

String Configuration::getPreference(const String key, const String& defaultValue)
{
#ifdef ESP32
    if (!prefs.isKey(key.c_str())) {
        return defaultValue;
    }
    return prefs.getString(key.c_str(), defaultValue);
#else
    return readVariableString(key, defaultValue);
#endif
}

String Configuration::getHostname()
{
    return getPreference("hostname", String(HOSTNAME));
}

String Configuration::getJsonConfig()
{
    String jsonString;
#ifdef ESP32
    JsonDocument doc;
    prefs.begin("config", true);

    auto keys = getPreferenceKeys();
    for (auto& key : keys) {
        int intVal = prefs.getInt(key.c_str(), INT_MIN);
        if (intVal != INT_MIN) {
            doc[key] = intVal;
        } else {
            String val = prefs.getString(key.c_str(), "__NO_STRING__");
            if (val != "__NO_STRING__") {
                doc[key] = val;
            } else {
                doc[key] = nullptr;
            }
        }
    }
    prefs.end();

    serializeJson(doc, jsonString);
#else
    serializeJson(json_preferences, jsonString);
#endif
    return jsonString;
}

bool Configuration::setJsonConfig(const String json)
{
#ifdef ESP32
    eventManager->debug("Not possible to set JSON config on ESP32", 0);
    return false;
#else
    json_preferences = json;
    return writeJsonPreferences();
#endif
}

std::map<String, String> Configuration::getPreferences()
{
    std::map<String, String> vars;

#ifdef ESP32
    prefs.begin("config", true);
    auto keys = getPreferenceKeys();

    for (auto& key : keys) {
        // Essaye int d'abord
        int intVal = prefs.getInt(key.c_str(), INT_MIN);
        if (intVal != INT_MIN) {
            vars[key] = String(intVal);
        } else {
            String val = prefs.getString(key.c_str(), "");
            vars[key] = val;
        }
    }
    prefs.end();

#else  // ESP8266
    for (JsonPair kv : json_preferences.as<JsonObject>()) {
        String key = kv.key().c_str();
        // Convertir la valeur en string (en fonction du type)
        if (kv.value().is<const char*>()) {
            vars[key] = String(kv.value().as<const char*>());
        } else if (kv.value().is<int>()) {
            vars[key] = String(kv.value().as<int>());
        } else if (kv.value().is<float>()) {
            vars[key] = String(kv.value().as<float>());
        } else {
            // fallback : serialize JSON value to string
            String tmp;
            serializeJson(kv.value(), tmp);
            vars[key] = tmp;
        }
    }
#endif

    return vars;
}

bool Configuration::saveProgramsJson(const String& json)
{
    return setPreference("device_programs", json);
}

bool Configuration::loadProgramsJson(String& outJson)
{
    outJson = getPreference("device_programs", "");
    return outJson.length() > 0;
}

#ifdef ESP8266

bool Configuration::readJsonPreferences()
{
    // Lire l'EEPROM et charger le JSON
    String jsonStr = "";
    for (size_t i = 0; i < eeprom.length(); i++) {
        char c = eeprom.read(i);
        if (c == '\0')
            break;
        jsonStr += c;
    }

    // Désérialiser le JSON
    DeserializationError error = deserializeJson(json_preferences, jsonStr);
    if (error) {
        Serial.print("Failed to deserialize JSON: ");
        Serial.println(error.c_str());
        return false;
    }

    Serial.println("Preferences loaded");
    Serial.println(jsonStr);

    return true;
}

bool Configuration::writeJsonPreferences()
{
    // Sérialiser le JSON
    String jsonStr;
    serializeJson(json_preferences, jsonStr);

    // Effacer l'EEPROM
    for (size_t i = 0; i < eeprom.length(); i++) {
        eeprom.write(i, '\0');
    }

    // Écrire le JSON dans l'EEPROM
    for (size_t i = 0; i < jsonStr.length(); i++) {
        eeprom.write(i, jsonStr[i]);
    }

    eeprom.commit();

    Serial.println("Preferences saved");
    Serial.println(jsonStr);
    return true;
}

bool Configuration::writeVariable(const String key, int value)
{
    json_preferences[key] = value;
    return writeJsonPreferences();
}

bool Configuration::writeVariable(const String key, String value)
{
    json_preferences[key] = value;
    return writeJsonPreferences();
}

int Configuration::readVariableInt(const String key, int defaultValue)
{
    //return json_preferences.containsKey(key) ? json_preferences[key].as<int>() : defaultValue;
    return json_preferences[key].is<int>() ? json_preferences[key].as<int>() : defaultValue;
}

String Configuration::readVariableString(const String key, String defaultValue)
{
    //return json_preferences.containsKey(key) ? json_preferences[key].as<String>() : defaultValue;
    return json_preferences[key].is<String>() ? json_preferences[key].as<String>() : defaultValue;
}

void Configuration::debugJsonPreferences()
{
    eventManager->debug("Preferences:", 1);
    String jsonString;
    serializeJsonPretty(json_preferences, jsonString);
    eventManager->debug(jsonString, 1);
}

#endif