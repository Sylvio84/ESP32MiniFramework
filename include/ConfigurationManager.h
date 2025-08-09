#ifndef CONFIGURATIONMANAGER_H
#define CONFIGURATIONMANAGER_H

#include <Manager.h>
#include <Arduino.h>
#include <map>

#ifdef ESP32
#include <Preferences.h>
#include <ArduinoJson.h>
#include <nvs.h>
#include <vector>
#else
#include <EEPROM.h>
#include <ArduinoJson.h>
#endif

/**
 * @brief ConfigurationManager - Centralized configuration and preferences management
 * 
 * This manager has completely replaced the legacy Configuration class and provides
 * a unified interface for all configuration needs in the ESP32MiniFramework.
 * 
 * ## Key Responsibilities:
 * - **Preference Management**: Persistent storage using NVS (ESP32) or EEPROM (ESP8266)
 * - **JSON Configuration**: Import/export full configuration as JSON
 * - **Device Settings**: Hostname, debug level, power saving mode
 * - **Migration**: Replaced the old Configuration class for consistency
 * 
 * ## Architecture:
 * - Extends the Manager base class following SOLID principles
 * - Implements getName() to return "ConfigurationManager" for DI container
 * - Handles configuration-related commands via onCommand()
 * - Thread-safe preference access methods
 * 
 * ## Usage:
 * ```cpp
 * auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
 * String ssid = configMgr->getPreference("wf_ssid", "default_ssid");
 * configMgr->setPreference("wf_ssid", "MyNetwork");
 * ```
 * 
 * ## Commands Handled:
 * - `config:debuglevel` - Get/set debug verbosity level (0-3)
 * - `config:hostname` - Get/set device hostname
 * - `config:power_saving` - Enable/disable power saving mode
 * - `config:json` - Export/import full configuration as JSON
 * 
 * ## Migration Notes:
 * - All references to Configuration class have been migrated to ConfigurationManager
 * - Use getManager("ConfigurationManager") instead of getConfiguration()
 * - All preference keys remain compatible with the old system
 */
class ConfigurationManager : public Manager
{
public:
    // === Configuration Constants ===
    int CONNECTION_TIMEOUT = 10000;

    const char *HOSTNAME = "ESP32";
    const char *OTA_HOST = "home.zore.org";
    const char *OTA_FINGERPRINT = "35 EF E8 CB CC 63 97 13 70 41 85 19 5C B3 CC 81 5A 79 C0 7A C1 1F 98 E6 1D D5 8B 98 23 50 B6 22";
    int OTA_PORT = 443;
    const char *OTA_URL = "esp/default/firmaware.bin"; // should be overriden in derived classes

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

    const char* OTA_CERT_PEM = nullptr;

    // === Manager Interface ===
    explicit ConfigurationManager(FrameworkContext& context);
    
    void init() override;
    String getName() const override { return "ConfigurationManager"; }
    
    bool onCommand(const String& command, const std::vector<String>& params) override;

    // === Configuration API (former Configuration class methods) ===
    static int getValue(const String key, int defaultValue = 0);
    static String getValue(const String key, String defaultValue = "");

    bool setPreference(const String key, int value);
    bool setPreference(const String key, String value);

    String getJsonConfig();
    bool setJsonConfig(const String json);

    std::map<String, String> getPreferences();

    int getPreference(const String key, int defaultValue = 0);
    String getPreference(const String key, const String &defaultValue = "");

    bool saveProgramsJson(const String& json);
    bool loadProgramsJson(String& outJson);

    String getHostname();

private:
    void registerCommands();

public:
#ifndef ESP32
#define EEPROM_PREFERENCES_SIZE 4096
    JsonDocument json_preferences;
    void debugJsonPreferences();
#endif

private:
    // === Power Saving Support ===
    void setPowerSaving(int value, bool save = true);
    
    // === Platform-specific Storage ===
#ifdef ESP32
    Preferences prefs;
    std::vector<String> getPreferenceKeys();
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

#endif // CONFIGURATIONMANAGER_H