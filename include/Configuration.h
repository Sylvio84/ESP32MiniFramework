#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <Arduino.h>
#include <EventManager.h>

#ifdef ESP32
#include <Preferences.h>
#else
#include <EEPROM.h>
#include <ArduinoJson.h>
#endif

class Configuration
{
public:
    int CONNECTION_TIMEOUT = 10000;

    const char *HOSTNAME = "ESP32";
    const char *OTA_HOST = "home.zore.org";
    const char *OTA_FINGERPRINT = "35 EF E8 CB CC 63 97 13 70 41 85 19 5C B3 CC 81 5A 79 C0 7A C1 1F 98 E6 1D D5 8B 98 23 50 B6 22";
    int OTA_PORT = 443;
    const char *OTA_URL = "esp/default/firmaware.bin"; // should be overriden in MyConfig

    int LCD_ADDRESS = 0x27;
    int LCD_COLS = 20;
    int LCD_ROWS = 4;
    int LCD_SDA = 13;
    int LCD_SCL = 14;

    const char *TIMEZONE = "CET-1CEST,M3.5.0,M10.5.0/3";
    const char *NTP_SERVER = "pool.ntp.org";
    const char *DATE_FORMAT = "%d/%m/%Y";
    const char *TIME_FORMAT = "%H:%M:%S";
    const char *DATETIME_FORMAT = "%d/%m/%Y %H:%M:%S";

    Configuration();

    void init(EventManager &eventMgr);

    static int getValue(const String key, int defaultValue = 0);

    static String getValue(const String key, String defaultValue = "");

    bool setPreference(const String key, int value);
    bool setPreference(const String key, String value);

    String getJsonConfig();
    bool setJsonConfig(const String json);

    int getPreference(const String key, int defaultValue = 0);
    String getPreference(const String key, const String &defaultValue = "");

    String getHostname();

#ifndef ESP32
#define EEPROM_PREFERENCES_SIZE 4096
    JsonDocument json_preferences;

    void debugJsonPreferences();
#endif

private:
    static EventManager *eventManager; // Pointeur vers EventManager

#ifdef ESP32
    Preferences prefs;
#else
    EEPROMClass eeprom;

    bool readJsonPreferences();
    bool writeJsonPreferences();

    bool writeVariable(const String key, int value);
    bool writeVariable(const String key, String value);
    int readVariableInt(const String key, int defaultValue = 0);
    String readVariableString(const String key, String defaultValue = "");

#endif
};

#endif
