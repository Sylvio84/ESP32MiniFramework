#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <Arduino.h>

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

    void init();

    static int getValue(const char *key, int defaultValue = 0);

    static String getValue(const char *key, String defaultValue = "");

    bool setPreference(const char *key, int value);
    bool setPreference(const char *key, String value);

    int getPreference(const char *key, int defaultValue = 0);
    String getPreference(const char *key, String defaultValue = "");

#ifndef ESP32
#define EEPROM_PREFERENCES_SIZE 4096
    JsonDocument json_preferences;

    void debugJsonPreferences();
#endif

private:
#ifdef ESP32
    Preferences prefs;
#else
    EEPROMClass eeprom;

    bool readJsonPreferences();
    bool writeJsonPreferences();

    bool writeVariable(const char *key, int value);
    bool writeVariable(const char *key, String value);
    int readVariableInt(const char *key, int defaultValue = 0);
    String readVariableString(const char *key, String defaultValue = "");

#endif
};

#endif
