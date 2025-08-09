#ifndef SYSTEMMANAGER_H
#define SYSTEMMANAGER_H

#include <Manager.h>
#include <Arduino.h>
#include <LittleFS.h>

#ifdef ESP32
#include <esp_chip_info.h>
#endif

/**
 * @brief SystemManager - System operations, power management, and control
 * 
 * Comprehensive system management including power saving, system information,
 * hardware control, and maintenance operations.
 * 
 * ## Key Responsibilities:
 * 
 * ### Power Management
 * - **Automatic Power Saving**: Reduces CPU frequency during idle periods
 * - **Suspend/Resume Events**: Responds to power_saving_suspend/resume events
 * - **Configurable Delays**: Adjustable loop delays based on power mode
 * - **Methods**: `setPowerSaving()`, `getPowerSaving()`, automatic delays in `loop()`
 * 
 * ### System Control
 * - **CPU Frequency**: Dynamic frequency adjustment (80/160/240 MHz on ESP32)
 * - **System Restart**: Safe restart with cleanup
 * - **OTA Updates**: Over-the-air firmware updates
 * - **LED Control**: Internal LED for status indication
 * 
 * ### System Information
 * - **Version Info**: Framework version and release date
 * - **Uptime Tracking**: System runtime in human-readable format
 * - **Temperature**: CPU temperature monitoring (ESP32 only)
 * - **Filesystem**: LittleFS usage statistics
 * - **Chip Info**: Model, cores, features, MAC address
 * 
 * ## Commands Handled:
 * - `system:version` - Display framework version and release date
 * - `system:uptime` - Show system uptime (days, hours, minutes, seconds)
 * - `system:temp` - Display CPU temperature (ESP32 only)
 * - `system:info` - Comprehensive system information dump
 * - `system:fs` - Filesystem usage (used/total bytes, percentage)
 * - `system:led [on|off|toggle]` - Control internal LED
 * - `system:freq [80|160|240]` - Set CPU frequency (ESP32 only)
 * - `system:restart` / `system:reboot` - Restart the system
 * - `system:ota [url]` - Trigger OTA firmware update
 * - `wifi:ssid [value]` - Get/set WiFi SSID (delegates to WiFiManager)
 * - `wifi:password [value]` - Get/set WiFi password (delegates to WiFiManager)
 * 
 * ## Event Handling:
 * - `sys:power_saving_suspend` - Temporarily disables power saving
 * - `sys:power_saving_resume` - Re-enables power saving after timeout
 * 
 * ## Architecture:
 * - Extends Manager base class following SOLID principles
 * - Implements getName() returning "SystemManager"
 * - Command delegation pattern for cross-manager operations
 * - Event-driven power management
 */
class SystemManager : public Manager
{
public:
    explicit SystemManager(FrameworkContext& context);
    
    void init() override;
    void loop() override;
    String getName() const override { return "SystemManager"; }
    
    bool onCommand(const String& command, const std::vector<String>& params) override;
    bool onEvent(const String& type, const String& event, const std::vector<String>& params) override;

private:
    // === Command Registration ===
    void registerCommands();
    // === Internal LED Control ===
    void internalLed(bool state);
    bool internalLedState();
    
    // === System Information ===
    void showSystemInfo();
    void showFilesystemInfo();
    
    // === CPU Frequency Control ===
    void setCpuFrequency(int frequency);
    int getCpuFrequency();
    
    // === Temperature Reading ===
    float getCpuTemperature();
    
public:
    // === Power Management ===
    void setPowerSaving(int value, bool save = true);
    int getPowerSaving() const { return powerSaving; }
    uint getPowerSavingResumeTimer() const { return powerSavingResumeTimer; }
    void clearPowerSavingResumeTimer();

private:
    
    // === System Control ===
    void restartSystem();
    void performOtaUpdate();
    
    // === Constants ===
    static const char* RELEASE_VERSION;
    static const char* RELEASE_DATE;

private:
    // === Power Management State ===
    int powerSaving = 0; // 0 = disabled, else = idle time in ms
    uint powerSavingResumeTimer = 0;
};

#endif // SYSTEMMANAGER_H