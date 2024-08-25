#include "../include/Configuration.h"

EventManager *Configuration::eventManager = nullptr;

#ifdef ESP32
Configuration::Configuration()
{
    prefs.begin("config", false);
}
#else
Configuration::Configuration() : json_preferences()
{
    eeprom.begin(EEPROM_PREFERENCES_SIZE);
}
#endif

void Configuration::init(EventManager &eventMgr)
{
    eventManager = &eventMgr;
#ifndef ESP32
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
    if (key.length() > 16 || key.length() == 0)
    {
        return false;
    }

#ifdef ESP32
    prefs.putInt(key, value);
    return true;
#else
    return writeVariable(key, value);
#endif
}

bool Configuration::setPreference(const String key, String value)
{
    if (key.length() > 16 || key.length() == 0 || value.length() > 255)
    {
        return false;
    }
#ifdef ESP32
    prefs.putString(key, value);
    return true;
#else
    return writeVariable(key, value);
#endif
}

int Configuration::getPreference(const String key, int defaultValue)
{
#ifdef ESP32
    return prefs.getInt(key, defaultValue);
#else
    return readVariableInt(key, defaultValue);
#endif
}

String Configuration::getPreference(const String key, const String& defaultValue)
{
#ifdef ESP32
    return prefs.getString(key, defaultValue);
#else
    return readVariableString(key, defaultValue);
#endif
}

#ifndef ESP32

bool Configuration::readJsonPreferences()
{
    // Lire l'EEPROM et charger le JSON
    String jsonStr = "";
    for (size_t i = 0; i < eeprom.length(); i++)
    {
        char c = eeprom.read(i);
        if (c == '\0')
            break;
        jsonStr += c;
    }

    // Désérialiser le JSON
    DeserializationError error = deserializeJson(json_preferences, jsonStr);
    if (error)
    {
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
    for (size_t i = 0; i < eeprom.length(); i++)
    {
        eeprom.write(i, '\0');
    }

    // Écrire le JSON dans l'EEPROM
    for (size_t i = 0; i < jsonStr.length(); i++)
    {
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
    return json_preferences.containsKey(key) ? json_preferences[key].as<int>() : defaultValue;
}

String Configuration::readVariableString(const String key, String defaultValue)
{
    return json_preferences.containsKey(key) ? json_preferences[key].as<String>() : defaultValue;
}

void Configuration::debugJsonPreferences()
{
    eventManager->debug("Preferences:", 1);
    String jsonString;
    serializeJsonPretty(json_preferences, jsonString);
    eventManager->debug(jsonString, 1);
}

#endif
