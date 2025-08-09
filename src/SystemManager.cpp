#include <SystemManager.h>
#include <WiFiManager.h>
#include <MQTTManager.h>
#include <TimeManager.h>
#include <ConfigurationManager.h>
#include <EventManager.h>
#include <CommandManager.h>
#include <Command.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

// Static constants
const char* SystemManager::RELEASE_VERSION = "1.1.0";
const char* SystemManager::RELEASE_DATE = "2025-08-06";

SystemManager::SystemManager(FrameworkContext& context) : Manager(context)
{
}

void SystemManager::init()
{
    debug("SystemManager init...", 1);
    
    // Initialize internal LED
    pinMode(LED_BUILTIN, OUTPUT);
    internalLed(false); // Start with LED off
    
    // Initialize power saving from configuration
    auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
    if (configMgr) {
        setPowerSaving(configMgr->getPreference("power_saving", 10));
    }
    
    // Set initial CPU frequency for power efficiency
#ifdef ESP32
    setCpuFrequencyMhz(80);
#endif
    
    // Register commands with CommandManager
    registerCommands();
    
    setInitialized(true);
}

void SystemManager::loop()
{
    // Handle power saving delay when conditions are met
    auto* wifiMgr = static_cast<WiFiManager*>(context->getManager("WiFiManager"));
    auto* mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
    auto* timeMgr = static_cast<TimeManager*>(context->getManager("TimeManager"));
    
    if (wifiMgr && wifiMgr->isConnected() && 
        mqttMgr && mqttMgr->isConnected() && 
        timeMgr && timeMgr->isInitialized && 
        powerSaving > 0) {
        delay(powerSaving);
    }
}

bool SystemManager::onCommand(const String& command, const std::vector<String>& params)
{
    // All commands are now handled by CommandManager
    // This method is kept for backward compatibility with event system
    debug("SystemManager::onCommand deprecated - use CommandManager", 2);
    return false;
}

bool SystemManager::onEvent(const String& type, const String& event, const std::vector<String>& params)
{
    if (type == "sys") {
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
                return onCommand(params[0], {});
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
            return false; // Let MainController handle this
        } else if (event == "command") {
            // Forward serial commands to ESPUI
            context->getEventManager()->triggerEvent("espui", "SerialIn", params);
            return true;
        }
        
    } else if (type == "telnet") {
        if (event == "input") {
            // Forward to input processing - this needs to be handled by MainController  
            return false; // Let MainController handle this
        }
    }
    
    return false; // Event not handled
}

// === Internal LED Control ===

void SystemManager::internalLed(bool state)
{
    digitalWrite(LED_BUILTIN, state ? LOW : HIGH); // LED is typically active low
}

bool SystemManager::internalLedState()
{
    return digitalRead(LED_BUILTIN) == LOW;
}

// === System Information ===

void SystemManager::showSystemInfo()
{
    debug("ESP32 Mini Framework Version: " + String(RELEASE_VERSION) + " (" + String(RELEASE_DATE) + ")", 0);
    debug("Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz", 0);
    
#ifdef ESP32
    debug("Total Heap: " + String(ESP.getHeapSize() / 1024) + " KB", 0);
#endif
    debug("Free Heap: " + String(ESP.getFreeHeap() / 1024) + " KB", 0);
    debug("Flash size: " + String(ESP.getFlashChipSize() / 1024) + " KB", 0);
    debug("Sketch size: " + String(ESP.getSketchSize() / 1024) + " KB", 0);
    debug("Free sketch space: " + String(ESP.getFreeSketchSpace() / 1024) + " KB", 0);
    
#ifdef ESP32
    debug("Chip ID: " + String(ESP.getEfuseMac()), 0);
    debug("Chip model: " + String(ESP.getChipModel()), 0);
    debug("Chip revision: " + String(ESP.getChipRevision()), 0);
    debug("Chip core: " + String(ESP.getChipCores()), 0);
#endif

#ifdef ESP8266
    debug("Reset reason: " + ESP.getResetReason(), 0);
#endif

    // Get information from other managers
    auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
    if (configMgr) {
        debug("Hostname: " + configMgr->getHostname(), 0);
        debug("Debug level: " + String(configMgr->getPreference("debug_level", 0)), 0);
        debug("Power saving time: " + String(configMgr->getPreference("power_saving", 0)), 0);
    }
    
    auto* timeMgr = static_cast<TimeManager*>(context->getManager("TimeManager"));
    if (timeMgr) {
        debug("Time: " + timeMgr->getFormattedDateTime("%d/%m/%Y %H:%M:%S"), 0);
    }
    
    auto* wifiMgr = static_cast<WiFiManager*>(context->getManager("WiFiManager"));
    if (wifiMgr) {
        if (wifiMgr->isConnected()) {
            debug("Connected to WiFi: " + wifiMgr->retrieveSSID(), 0);
            debug("IP address: " + wifiMgr->retrieveIP(), 0);
        } else {
            debug("Not connected to WiFi", 0);
        }
    }
    
    auto* mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
    if (mqttMgr) {
        if (mqttMgr->isConnected()) {
            debug("Connected to MQTT server: " + mqttMgr->retrieveServer(), 0);
        } else {
            debug("Not connected to MQTT server", 0);
        }
    }

#ifdef ESP8266
    debug("Power saving: " + String(wifi_get_sleep_type() == NONE_SLEEP_T ? "disabled" : "enabled"), 0);
#endif
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

// === Command Registration ===

void SystemManager::registerCommands()
{
    auto* cmdMgr = static_cast<CommandManager*>(context->getManager("CommandManager"));
    if (!cmdMgr) {
        debug("CommandManager not available for command registration", 1);
        return;
    }
    
    // Version command
    cmdMgr->registerCommand(Command(
        "sys", "version", "Show framework version",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) {
            return "Version: " + String(RELEASE_VERSION) + " (" + String(RELEASE_DATE) + ")";
        }
    ));
    
    // Uptime command
    cmdMgr->registerCommand(Command(
        "sys", "uptime", "Show system uptime",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) {
            return "Uptime: " + String(millis() / 1000) + " seconds";
        }
    ));
    
    // Temperature command
    cmdMgr->registerCommand(Command(
        "sys", "temp", "Show CPU temperature",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) {
#ifdef ESP32
            float temp = getCpuTemperature();
            return "Temperature: " + String(temp) + "°C";
#else
            return "Temperature not supported on this device";
#endif
        }
    ));
    
    // LED command
    cmdMgr->registerCommand(Command(
        "sys", "led", "Control internal LED (on/off)",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                if (args[0] == "on") {
                    internalLed(true);
                    return String("LED on");
                } else if (args[0] == "off") {
                    internalLed(false);
                    return String("LED off");
                } else {
                    return String("Invalid parameter. Usage: led on|off");
                }
            } else {
                return String("LED state: " + String(internalLedState() ? "on" : "off"));
            }
        }
    ));
    
    // CPU Frequency command
    cmdMgr->registerCommand(Command(
        "sys", "freq", "Get/Set CPU frequency (80/160/240 MHz)",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
#ifdef ESP32
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
#else
            return String("Frequency command not supported on this device");
#endif
        }
    ));
    
    // System info command
    cmdMgr->registerCommand(Command(
        "sys", "info", "Show comprehensive system information",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            showSystemInfo();
            return String("System info displayed");
        }
    ));
    
    // Filesystem info command
    cmdMgr->registerCommand(Command(
        "sys", "fs", "Show filesystem information",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            showFilesystemInfo();
            return String("Filesystem info displayed");
        }
    ));
    
    // Restart command
    cmdMgr->registerCommand(Command(
        "sys", "restart", "Restart the system",
        CommandSource::Any, true,  // Enable history for restart
        [this](const std::vector<String>& args) -> String {
            debug("Restarting system...", 1);
            delay(500);
            restartSystem();
            return String("Restarting...");
        }
    ));
    
    // Reboot alias
    cmdMgr->registerAlias("reboot", "sys:restart");
    
    // OTA Update command
    cmdMgr->registerCommand(Command(
        "sys", "ota", "Start OTA firmware update",
        CommandSource::Serial,  // Only from Serial for security
        true,  // Enable history
        [this](const std::vector<String>& args) -> String {
            debug("Starting OTA update...", 1);
            performOtaUpdate();
            return String("OTA update started");
        }
    ));
    
    
    // WiFi configuration commands
    cmdMgr->registerCommand(Command(
        "wifi", "ssid", "Get/Set WiFi SSID",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
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
        }
    ));
    
    cmdMgr->registerCommand(Command(
        "wifi", "pass", "Get/Set WiFi password",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
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
        }
    ));
    
    cmdMgr->registerCommand(Command(
        "wifi", "connect", "Connect to WiFi with saved credentials",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            auto* wifiMgr = static_cast<WiFiManager*>(context->getManager("WiFiManager"));
            if (!wifiMgr) {
                return String("WiFiManager not available");
            }
            
            wifiMgr->autoConnect();
            return String("WiFi connection initiated");
        }
    ));
    
    cmdMgr->registerCommand(Command(
        "wifi", "status", "Show WiFi connection status",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
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
        }
    ));
    
    // LED Control Commands (for internal LED)
    cmdMgr->registerCommand(Command(
        "sys", "led", "Control internal LED (on/off/toggle/status)",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            if (args.empty()) {
                return String("Usage: sys:led <on|off|toggle|status>");
            }
            
            String action = args[0];
            action.toLowerCase();
            
            if (action == "on") {
                #ifdef ESP32
                    #ifdef CHIP_ESP32C3
                        pinMode(8, OUTPUT);
                        digitalWrite(8, LOW);  // LED on (inverted logic)
                    #else
                        pinMode(2, OUTPUT);
                        digitalWrite(2, LOW);  // LED on (inverted logic)
                    #endif
                #else
                    pinMode(LED_BUILTIN, OUTPUT);
                    digitalWrite(LED_BUILTIN, HIGH);
                #endif
                return String("Internal LED turned ON");
                
            } else if (action == "off") {
                #ifdef ESP32
                    #ifdef CHIP_ESP32C3
                        pinMode(8, OUTPUT);
                        digitalWrite(8, HIGH);  // LED off (inverted logic)
                    #else
                        pinMode(2, OUTPUT);
                        digitalWrite(2, HIGH);  // LED off (inverted logic)
                    #endif
                #else
                    pinMode(LED_BUILTIN, OUTPUT);
                    digitalWrite(LED_BUILTIN, LOW);
                #endif
                return String("Internal LED turned OFF");
                
            } else if (action == "toggle") {
                #ifdef ESP32
                    #ifdef CHIP_ESP32C3
                        pinMode(8, OUTPUT);
                        int currentState = digitalRead(8);
                        digitalWrite(8, !currentState);
                    #else
                        pinMode(2, OUTPUT);
                        int currentState = digitalRead(2);
                        digitalWrite(2, !currentState);
                    #endif
                #else
                    pinMode(LED_BUILTIN, OUTPUT);
                    int currentState = digitalRead(LED_BUILTIN);
                    digitalWrite(LED_BUILTIN, !currentState);
                #endif
                return String("Internal LED toggled");
                
            } else if (action == "status") {
                #ifdef ESP32
                    #ifdef CHIP_ESP32C3
                        int state = digitalRead(8);
                        return String("Internal LED (pin 8): ") + (state == LOW ? "ON" : "OFF");
                    #else
                        int state = digitalRead(2);
                        return String("Internal LED (pin 2): ") + (state == LOW ? "ON" : "OFF");
                    #endif
                #else
                    int state = digitalRead(LED_BUILTIN);
                    return String("Internal LED: ") + (state == HIGH ? "ON" : "OFF");
                #endif
                
            } else {
                return String("Invalid LED action. Use: on, off, toggle, or status");
            }
        }
    ));
    
    debug("System, WiFi and LED commands registered", 2);
}