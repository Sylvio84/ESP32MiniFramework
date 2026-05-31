#include <Command.h>
#include "Managers/CommandManager.h"
#include "Managers/ConfigurationManager.h"
#include "Managers/DeviceManager.h"
#include <Devices/InternalLedDevice.h>
#include "Managers/EventManager.h"
#include "Managers/MQTTManager.h"
#include "Managers/SystemManager.h"
#include "Managers/TimeManager.h"
#include "Managers/WiFiManager.h"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

// Static constants
const char* SystemManager::RELEASE_VERSION = "1.3.0";
const char* SystemManager::RELEASE_DATE = "2026-05-31";

SystemManager::SystemManager(FrameworkContext& context) : Manager(context) {}

void SystemManager::init()
{
    debug("SystemManager init...", 1);

    // Get LED device from DeviceManager
    auto* ledDev = getLedDevice();
    if (ledDev) {
        // Initialize internal LED
        ledDev->setState(false);  // Start with LED off
    }

    // Initialize power saving and debug level from configuration
    auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
    if (configMgr) {
        setPowerSaving(configMgr->getPreference("power_saving", 10));
        debugLevel = configMgr->getPreference("debug_level", 0);
    }

    // Set initial CPU frequency for power efficiency
#ifdef ESP32
    setCpuFrequencyMhz(80);
#endif

    // Register commands with CommandManager
    registerCommands();

    setInitialized(true);
    if (ledDev) {
        ledDev->startBlinkPattern(50, 500);
    }
}

void SystemManager::loop()
{
    // LED blink pattern is now handled by InternalLedDevice's loop()

    // Handle power saving delay when conditions are met
    auto* wifiMgr = static_cast<WiFiManager*>(context->getManager("WiFiManager"));
    auto* mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
    auto* timeMgr = static_cast<TimeManager*>(context->getManager("TimeManager"));

    if (wifiMgr && wifiMgr->isConnected() && mqttMgr && mqttMgr->isConnected() && timeMgr && timeMgr->isInitialized && powerSaving > 0) {
        delay(powerSaving);
    }
}


bool SystemManager::onEvent(const String& type, const String& event, const std::vector<String>& params)
{
    if (type == "wifi") {
        if (event == "connected" || event == "recovered") {
            // WiFi connected: slow blink (1s interval) for 3s max
            auto* ledDev = getLedDevice();
            if (ledDev) {
                ledDev->startBlinkPattern(500, 3000);
            }
            return false;  // Let other managers also handle this event
        }
    } else if (type == "mqtt") {
        if (event == "Connected") {
            // MQTT connected: fast blink (250ms interval) for 5s max
            auto* ledDev = getLedDevice();
            if (ledDev) {
                ledDev->startBlinkPattern(100, 5000);
            }
            return false;  // Let other managers also handle this event
        }
    } else if (type == "sys") {
        if (event.startsWith("@")) {
            // Handle system commands via events using CommandManager
            String command = event.substring(1);
            auto* cmdMgr = static_cast<CommandManager*>(context->getManager("CommandManager"));
            if (cmdMgr) {
                String result = cmdMgr->executeCommand("sys:" + command, params, CommandSource::Internal);
                if (!result.startsWith("Command not found")) {
                    debug("Command result: " + result, 2);
                    return true;
                }
            }
            return false;
        } else if (event == "power_saving_suspend") {
            clearPowerSavingResumeTimer();
            if (powerSaving > 0) {
                setPowerSaving(0, false);
                if (params.size() > 0) {
                    debug(params[0], 0);
                }
            }
            return true;

        } else if (event == "power_saving_resume") {
            clearPowerSavingResumeTimer();

            if (params.size() > 0) {
                auto* timeMgr = static_cast<TimeManager*>(context->getManager("TimeManager"));

                if (params.size() > 1 && isInteger(params[1]) && timeMgr) {
                    int duration = params[1].toInt() * 1000;
                    powerSavingResumeTimer = timeMgr->setTimeout(
                        [this, params] {
                            if (params[0].length() > 0) {
                                debug(params[0], 0);
                            }
                            setPowerSaving(-1, false);
                        },
                        duration);
                } else {
                    if (params[0].length() > 0) {
                        debug(params[0], 0);
                    }
                    setPowerSaving(-1, false);
                }
            } else {
                setPowerSaving(-1, false);
            }
            return true;
        }

    } else if (type == "espui") {
        if (event == "Command") {
            // Handle ESPUI commands via command system
            if (params.size() > 0) {
                auto* cmdMgr = static_cast<CommandManager*>(context->getManager("CommandManager"));
                if (cmdMgr) {
                    cmdMgr->executeCommand(params[0], {}, CommandSource::Web);
                    return true;
                }
            }
            return true;
        } else if (event == "Reboot") {
            debug("Restarting (espui event)...", 1);
            restartSystem();
            return true;
        }

    } else if (type == "serial") {
        if (event == "input") {
            // Forward to input processing - this needs to be handled by MainController
            return false;  // Let MainController handle this
        } else if (event == "command") {
            // Forward serial commands to ESPUI
            context->getEventManager()->triggerEvent("espui", "SerialIn", params);
            return true;
        }

    } else if (type == "telnet") {
        if (event == "input") {
            // Forward to input processing - this needs to be handled by MainController
            return false;  // Let MainController handle this
        }
    }

    return false;  // Event not handled
}

// === Internal LED Control ===

InternalLedDevice* SystemManager::getLedDevice()
{
    if (!ledDevice) {
        auto* devMgr = static_cast<DeviceManager*>(context->getManager("DeviceManager"));
        if (devMgr) {
            Device* dev = devMgr->getDeviceById("led");
            if (dev) {
                ledDevice = static_cast<InternalLedDevice*>(dev);
            }
        }
    }
    return ledDevice;
}

String SystemManager::getSystemInfo()
{
    String info = "";
    info += "ESP32 Mini Framework Version: " + String(RELEASE_VERSION) + " (" + String(RELEASE_DATE) + ")\n";
    info += "Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz\n";

#ifdef ESP32
    info += "Total Heap: " + String(ESP.getHeapSize() / 1024) + " KB\n";
#endif
    info += "Free Heap: " + String(ESP.getFreeHeap()) + " / " + String(ESP.getFreeHeap() / 1024) + " KB\n";
    info += "Flash size: " + String(ESP.getFlashChipSize() / 1024) + " KB\n";
    info += "Sketch size: " + String(ESP.getSketchSize() / 1024) + " KB\n";
    info += "Free sketch space: " + String(ESP.getFreeSketchSpace() / 1024) + " KB\n";

#ifdef ESP32
    info += "Chip ID: " + String(ESP.getEfuseMac()) + "\n";
    info += "Chip model: " + String(ESP.getChipModel()) + "\n";
    info += "Chip revision: " + String(ESP.getChipRevision()) + "\n";
    info += "Chip cores: " + String(ESP.getChipCores()) + "\n";
#endif

#ifdef ESP8266
    info += "Reset reason: " + ESP.getResetReason() + "\n";
    // Additional memory monitoring for ESP8266
    info += "Heap Fragmentation: " + String(ESP.getHeapFragmentation()) + "%\n";
    info += "Max Free Block Size: " + String(ESP.getMaxFreeBlockSize()) + " bytes\n";
    info += "Free Cont Stack: " + String(ESP.getFreeContStack()) + " bytes\n";
    
    // Flash information
    uint32_t realSize = ESP.getFlashChipRealSize();
    uint32_t ideSize = ESP.getFlashChipSize();
    if (realSize != ideSize) {
        info += "Flash Real Size: " + String(realSize / 1024) + " KB\n";
        info += "Flash IDE Size: " + String(ideSize / 1024) + " KB (mismatch!)\n";
    }
#endif

    // Get information from other managers
    auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
    if (configMgr) {
        info += "Hostname: " + configMgr->getHostname() + "\n";
    }
    info += "Debug level: " + String(debugLevel) + "\n";
    info += "Power saving time: " + String(powerSaving) + "\n";

    auto* timeMgr = static_cast<TimeManager*>(context->getManager("TimeManager"));
    if (timeMgr) {
        info += "Time: " + timeMgr->getFormattedDateTime("%d/%m/%Y %H:%M:%S") + "\n";
    }

    auto* wifiMgr = static_cast<WiFiManager*>(context->getManager("WiFiManager"));
    if (wifiMgr) {
        if (wifiMgr->isConnected()) {
            info += "Connected to WiFi: " + wifiMgr->retrieveSSID() + "\n";
            info += "IP address: " + wifiMgr->retrieveIP() + "\n";
        } else {
            info += "Not connected to WiFi\n";
        }
    }

    auto* mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
    if (mqttMgr) {
        if (mqttMgr->isConnected()) {
            info += "Connected to MQTT server: " + mqttMgr->retrieveServer() + "\n";
        } else {
            info += "Not connected to MQTT server\n";
        }
    }

#ifdef ESP8266
    info += "Power saving: " + String(wifi_get_sleep_type() == NONE_SLEEP_T ? "disabled" : "enabled") + "\n";
#endif

    // Remove trailing newline
    if (info.endsWith("\n")) {
        info.remove(info.length() - 1);
    }

    return info;
}

void SystemManager::showFilesystemInfo()
{
    if (!LittleFS.begin()) {
        debug("Failed to initialize LittleFS", 0);
        return;
    }

#ifdef ESP8266
    FSInfo fs_info;
    LittleFS.info(fs_info);
    size_t totalBytes = fs_info.totalBytes;
    size_t usedBytes = fs_info.usedBytes;
#endif

#ifdef ESP32
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
#endif

    debug("Total bytes: " + String(totalBytes), 0);
    debug("Used bytes: " + String(usedBytes), 0);
    debug("Free bytes: " + String(totalBytes - usedBytes), 0);
}

String SystemManager::getFilesystemInfo()
{
    if (!LittleFS.begin()) {
        return "Failed to initialize LittleFS";
    }

#ifdef ESP8266
    FSInfo fs_info;
    LittleFS.info(fs_info);
    size_t totalBytes = fs_info.totalBytes;
    size_t usedBytes = fs_info.usedBytes;
#endif

#ifdef ESP32
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
#endif

    String info = "";
    info += "Total bytes: " + String(totalBytes) + "\n";
    info += "Used bytes: " + String(usedBytes) + "\n";
    info += "Free bytes: " + String(totalBytes - usedBytes);

    return info;
}

// === CPU Control ===

void SystemManager::setCpuFrequency(int frequency)
{
#ifdef ESP32
    setCpuFrequencyMhz(frequency);
#endif
}

int SystemManager::getCpuFrequency()
{
    return ESP.getCpuFreqMHz();
}

float SystemManager::getCpuTemperature()
{
#ifdef ESP32
    return temperatureRead();
#else
    return 0.0;
#endif
}

// === System Control ===

void SystemManager::restartSystem()
{
    ESP.restart();
}

void SystemManager::performOtaUpdate()
{
    debug("Performing OTA update...", 0);
    delay(1000);
    // clear commands
    auto* cmdMgr = static_cast<CommandManager*>(context->getManager("CommandManager"));
    if (cmdMgr) {
        cmdMgr->clearCommands();
    }

    // disconnect MQTT
    auto* mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
    if (mqttMgr && mqttMgr->isConnected()) {
        mqttMgr->disconnect();
    }

    auto* wifiMgr = static_cast<WiFiManager*>(context->getManager("WiFiManager"));
    if (wifiMgr) {
        wifiMgr->otaUpdate();
    } else {
        debug("WiFiManager not available for OTA update", 0);
    }
}

// === Power Management ===

void SystemManager::setPowerSaving(int value, bool save)
{
    auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
    auto* wifiMgr = static_cast<WiFiManager*>(context->getManager("WiFiManager"));

    if (value < 0) {
        value = configMgr ? configMgr->getPreference("power_saving", 0) : 0;
    }
    if (value == 1) {
        value = 100;  // default value
    }

    powerSaving = value;

    if (powerSaving > 0) {
        debug("Power saving enabled: process every " + String(powerSaving) + "ms", 1);
        if (wifiMgr) {
            wifiMgr->setPowerSave(true);
        }
    } else {
        if (save) {
            debug("Power saving disabled", 1);
        } else {
            debug("Power saving suspended", 3);
        }
        if (wifiMgr) {
            wifiMgr->setPowerSave(false);
        }
    }

    if (save && configMgr) {
        configMgr->setPreference("power_saving", powerSaving);
    }
}

void SystemManager::clearPowerSavingResumeTimer()
{
    auto* timeMgr = static_cast<TimeManager*>(context->getManager("TimeManager"));
    if (timeMgr && powerSavingResumeTimer > 0) {
        timeMgr->clearTimeout(powerSavingResumeTimer);
        powerSavingResumeTimer = 0;
    }
}

// === Debug Level Management ===

void SystemManager::setDebugLevel(int level, bool save)
{
    debugLevel = level;
    debug("Debug level set to: " + String(debugLevel), 1);
    
    if (save) {
        auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
        if (configMgr) {
            configMgr->setPreference("debug_level", debugLevel);
        }
    }
}

// === Command Registration ===

void SystemManager::registerCommands()
{
    auto* cmdMgr = static_cast<CommandManager*>(context->getManager("CommandManager"));
    if (!cmdMgr) {
        debug("CommandManager not available for command registration", 1);
        return;
    }

    // Version command
    cmdMgr->registerCommand(Command("sys", "version", "Framework version", CommandSource::Any, true, [this](const std::vector<String>& args) {
        return "Version: " + String(RELEASE_VERSION) + " (" + String(RELEASE_DATE) + ")";
    }));

    #ifndef ESP8266
    // Uptime command
    cmdMgr->registerCommand(Command("sys", "uptime", "System uptime", CommandSource::Any, true,
                                    [this](const std::vector<String>& args) { return "Uptime: " + String(millis() / 1000) + " seconds"; }));

    // Echo command
    cmdMgr->registerCommand(Command("sys", "echo", "Echo back the provided text", CommandSource::Any, true, [this](const std::vector<String>& args) {
        if (args.size() == 0) {
            return String("");
        }
        String result = "";
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0)
                result += " ";
            result += args[i];
        }
        return result;
    }));

    // Temperature command
    cmdMgr->registerCommand(Command("sys", "temp", "CPU temp.", CommandSource::Any, true, [this](const std::vector<String>& args) {
        float temp = getCpuTemperature();
        return "Temperature: " + String(temp) + "°C";
    }));

    // LED command - now delegates to InternalLedDevice
    cmdMgr->registerCommand(Command("sys", "led", "Control internal LED (on/off)", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
        auto* ledDev = getLedDevice();
        if (!ledDev) {
            return String("LED device not available");
        }

        if (args.size() > 0) {
            if (args[0] == "on") {
                ledDev->setState(true);
                return String("LED on");
            } else if (args[0] == "off") {
                ledDev->setState(false);
                return String("LED off");
            } else {
                return String("Invalid parameter. Usage: led on|off");
            }
        } else {
            return String("LED state: " + String(ledDev->isOn() ? "on" : "off"));
        }
    }));

    // CPU Frequency command
    cmdMgr->registerCommand(
        Command("sys", "freq", "Get/Set CPU frequency (80/160/240 MHz)", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                int freq = args[0].toInt();
                if (freq == 80 || freq == 160 || freq == 240) {
                    if (freq == 240 && ESP.getChipModel() == "ESP32C3") {
                        return String("240 MHz frequency not supported on ESP32-C3");
                    }
                    setCpuFrequency(freq);
                    return String("Frequency set to: " + String(freq) + " MHz");
                } else {
                    return String("Invalid frequency. Valid: 80, 160, 240 MHz");
                }
            } else {
                int freq = getCpuFrequency();
                return String("CPU Frequency: " + String(freq) + " MHz");
            }
        }));

    // Filesystem info command
    cmdMgr->registerCommand(Command("sys", "fs", "Filesystem info", CommandSource::Any, true,
                                    [this](const std::vector<String>& args) -> String { return getFilesystemInfo(); }));

    #endif

    // System info command
    cmdMgr->registerCommand(Command("sys", "info", "System info", CommandSource::Any, true,
                                    [this](const std::vector<String>& args) -> String { return getSystemInfo(); }));

    // Restart command
    cmdMgr->registerCommand(Command("sys", "restart", "Reboot", CommandSource::Any, true,  // Enable history for restart
                                    [this](const std::vector<String>& args) -> String {
                                        debug("Restarting system...", 1);
                                        delay(500);
                                        restartSystem();
                                        return String("Restarting...");
                                    }));

    // Reboot alias
    cmdMgr->registerAlias("reboot", "sys:restart");

    // OTA Update command
    cmdMgr->registerCommand(Command("sys", "ota", "OTA FW update",
                                    CommandSource::Any,  // Allow from any source (Serial, MQTT, etc.)
                                    true,                   // Enable history
                                    [this](const std::vector<String>& args) -> String {
                                        performOtaUpdate();
                                        return String("OTA update started");
                                    }));

    // WiFi configuration commands
    cmdMgr->registerCommand(Command("wifi", "ssid", "Get/Set WiFi SSID", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
        auto* wifiMgr = static_cast<WiFiManager*>(context->getManager("WiFiManager"));
        if (!wifiMgr) {
            return String("WiFiManager not available");
        }

        if (args.size() > 0) {
            // Set SSID
            wifiMgr->saveSSID(args[0], false);
            return String("WiFi SSID saved: " + args[0]);
        } else {
            // Get SSID
            String ssid = wifiMgr->retrieveSSID();
            if (ssid.isEmpty()) {
                return String("WiFi SSID: (not set)");
            } else {
                return String("WiFi SSID: " + ssid);
            }
        }
    }));

    cmdMgr->registerCommand(Command("wifi", "pass", "Get/Set WiFi password", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
        auto* wifiMgr = static_cast<WiFiManager*>(context->getManager("WiFiManager"));
        if (!wifiMgr) {
            return String("WiFiManager not available");
        }

        if (args.size() > 0) {
            // Set password
            wifiMgr->savePassword(args[0], false);
            return String("WiFi password saved");
        } else {
            // Get password (masked for security)
            String pass = wifiMgr->retrievePassword();
            if (pass.isEmpty()) {
                return String("WiFi password: (not set)");
            } else {
                return String("WiFi password: ****");
            }
        }
    }));

    cmdMgr->registerCommand(
        Command("wifi", "connect", "Connect to WiFi", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
            auto* wifiMgr = static_cast<WiFiManager*>(context->getManager("WiFiManager"));
            if (!wifiMgr) {
                return String("WiFiManager not available");
            }

            wifiMgr->autoConnect();
            return String("WiFi connection initiated");
        }));

    cmdMgr->registerCommand(
        Command("wifi", "status", "WiFi conn. status", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
            auto* wifiMgr = static_cast<WiFiManager*>(context->getManager("WiFiManager"));
            if (!wifiMgr) {
                return String("WiFiManager not available");
            }

            String result = "WiFi Status:\n";
            result += "  Connected: " + String(wifiMgr->isConnected() ? "Yes" : "No") + "\n";
            result += "  SSID: " + wifiMgr->retrieveSSID() + "\n";
            result += "  Status: " + wifiMgr->getStatus() + "\n";
            if (wifiMgr->isConnected()) {
                result += "  IP: " + wifiMgr->retrieveIP();
            }
            return result;
        }));

    // Debug level command
    cmdMgr->registerCommand(Command("sys", "debug", "Get/Set debug level (0-3)", CommandSource::Any, true, 
        [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                int level = args[0].toInt();
                if (level >= 0 && level <= 9) {
                    setDebugLevel(level);
                    return String("Debug level set to: " + String(level));
                } else {
                    return String("Invalid debug level. Valid range: 0-9");
                }
            } else {
                return String("Debug level: " + String(debugLevel));
            }
        }));

    /*cmdMgr->registerCommand(Command("sys", "clearmem", "Clear memory caches and perform garbage collection", CommandSource::Any, true,
       [this](const std::vector<String>& args) -> String {
           // Clear MQTT subscriptions history if too large
           auto* mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
           if (mqttMgr) {
               // Clear retained publications not needed
               std::vector<String> subs = mqttMgr->getSubscriptions();
               for (const auto& sub : subs) {
                   mqttMgr->removePublication(sub);
               }
           }
           
           // Clear event manager callbacks that are no longer needed
           //auto* evtMgr = static_cast<EventManager*>(context->getManager("EventManager"));
           
           // Force String pool cleanup
           #ifdef ESP8266
           // ESP8266 specific memory cleanup
           ESP.wdtFeed();
           yield();
           #elif defined(ESP32)
           // ESP32 specific cleanup
           heap_caps_malloc_extmem_enable(0);
           #endif
           
           // Report memory status
           size_t freeBefore = ESP.getFreeHeap();
           
           // Return memory status
           return "Memory cleaned. Free heap: " + String(freeBefore) + " bytes";
       }
    ));*/
}