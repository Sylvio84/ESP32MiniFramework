#include "../include/MainController.h"

MainController::MainController(Configuration& config)
    : eventManager(),
      config(config),
      serialCommandManager(config, eventManager),
      displayManager(config),
      wiFiManager(config, eventManager),
      mqttManager(config, eventManager),
      timeManager(config, eventManager)
#ifndef DISABLE_ESPUI
      ,
      espUIManager(config, eventManager)
#endif
{
    eventManager.registerMainCallback(
        [this](const String& eventType, const String& event, const std::vector<String>& params) { this->processEvent(eventType, event, params); });
    eventManager.registerDebugCallback(
        [this](const String& message, int level, bool displayTime = true) { this->processDebugMessage(message, level, displayTime); });
}

void MainController::init()
{
    delay(500);
    config.init(eventManager);

    serialCommandManager.init();
    displayManager.init();
    wiFiManager.init();
    timeManager.init();
    mqttManager.init();
    mqttManager.addSubscription(config.getHostname() + "/cmd");
#ifndef DISABLE_ESPUI
    espUIManager.init();
    wiFiManager.initEspUI();
    mqttManager.initEspUI();
#endif
    for (auto& device : devices) {
        device->init();
    }

    displayManager.clear();
    if (wiFiManager.isConnected()) {
        displayManager.printLine(0, "Wifi Connected");
    } else {
        displayManager.printLine(0, "Not connected");
    }

    eventManager.debug("Init done!", 1);
    eventManager.debug("Welcome on " + config.getHostname() + "!", 0);
    setPowerSaving(config.getPreference("power_saving", 10));
}

void MainController::loop()
{
    serialCommandManager.loop();
    timeManager.loop();
    wiFiManager.loop();
    if (wiFiManager.isConnected()) {
        mqttManager.loop();
        if (mqttManager.isConnected() && timeManager.isInitialized && powerSaving > 0) {
            delay(powerSaving);
        }
    }
    for (auto& device : devices) {
        device->loop();
    }
}

#ifndef DISABLE_ESPUI
void MainController::processUI(String action, std::vector<String> params)
{
    eventManager.debug("Processing UI: " + action, 3);
    for (const auto& param : params) {
        eventManager.debug("Param: " + param, 3);
    }
    for (auto& device : devices) {
        device->processUI(action, params);
    }

    displayManager.printLine(1, action.c_str());
    if (action == "WiFiConnect") {
        wiFiManager.connect();
    } else if (action == "WiFiDisconnect") {
        wiFiManager.disconnect();
    } else if (action == "WiFiHotspot") {
        wiFiManager.startAccessPoint();
    } else if (action == "WiFiAutoConnect") {
        wiFiManager.autoConnect();
    } else if (action == "WiFiSaveSSID") {
        wiFiManager.saveSSID(params[0]);
    } else if (action == "WiFiSavePassword") {
        wiFiManager.savePassword(params[0]);
    } else if (action == "MQTTSaveServer") {
        mqttManager.saveServer(params[0]);
    } else if (action == "MQTTSavePort") {
        mqttManager.savePort(params[0].toInt());
    } else if (action == "MQTTSaveUser") {
        mqttManager.saveUsername(params[0]);
    } else if (action == "MQTTSavePass") {
        mqttManager.savePassword(params[0]);
    } else if (action == "MQTTReconnect") {
        mqttManager.reconnect();
    } else if (action == "Reboot") {
        eventManager.debug("Restarting (espui)...", 1);
        ESP.restart();
    } else if (action == "DisplayClear") {
        displayManager.clear();
    } else if (action == "DisplayPrintLine") {
        int line = params[0].toInt();
        displayManager.printLine(line, params[1].c_str());
    } else if (action == "WiFiConnected") {
        eventManager.debug("Connected to WiFi: " + params[0], 1);
        eventManager.debug("IP address: " + params[1], 1);
        timeManager.update();
    }
}
#endif

void MainController::processEvent(String type, String event, std::vector<String> params)
{
    eventManager.debug("Processing Event: " + type + " / " + event, 3);
    for (const auto& param : params) {
        eventManager.debug("Param: " + param, 3);
    }

    wiFiManager.processEvent(type, event, params);
    mqttManager.processEvent(type, event, params);
#ifndef DISABLE_ESPUI
    espUIManager.processEvent(type, event, params);
#endif
    // displayManager.processEvent(type, event, params);
    for (auto& device : devices) {
        device->processEvent(type, event, params);
    }
    if (type == "wifi") {
        if (event == "connected" || event == "recovered") {
            eventManager.debug("Connected to WiFi: " + params[0], 1);
            eventManager.debug("IP address: " + params[1], 1);
            timeManager.update();
            mqttManager.setStatus(2);
            // ESPUI.server->reset(); // Remove all handlers and writers // ESPUI.server->end();
        }
        if (event == "ap_started") {
            // ESPUI.begin();
        }
        if (event == "disconnected" || event == "lost") {
            mqttManager.setStatus(1);
        }
    } else if (type == "sys") {
        if (event.startsWith("@")) {
            processCommand(event.substring(1), params);
        }
        if (event == "power_saving_suspend") {
            if (powerSavingRemumeTimer > 0) {
                timeManager.clearTimeout(powerSavingRemumeTimer);
            }
            if (powerSaving > 0) {
                setPowerSaving(0, false);
                if (params.size() > 0) {
                    eventManager.debug(params[0], 0);
                }
            }
        }
        if (event == "power_saving_resume") {
            if (powerSavingRemumeTimer > 0) {
                timeManager.clearTimeout(powerSavingRemumeTimer);
            }
            if (params.size() > 0) {

                if (params.size() > 1 && isInteger(params[1])) {
                    int duration = params[1].toInt() * 1000;
                    powerSavingRemumeTimer = timeManager.setTimeout(
                        [this, params] {
                            if (params[0].length() > 0) {
                                eventManager.debug(params[0], 0);
                            }
                            setPowerSaving(-1, false);
                        },
                        duration);
                } else {
                    if (params[0].length() > 0) {
                        eventManager.debug(params[0], 0);
                    }
                    setPowerSaving(-1, false);
                }
            } else {
                setPowerSaving(-1, false);
            }
        }
    } else if (type == "mqtt") {
        if (event == "connected") {
            eventManager.debug("Connected to MQTT server: " + params[0], 1);
        } else if (event == "message") {
            processMQTT(params[0], params[1]);
        }
#ifndef DISABLE_ESPUI
    } else if (type == "espui") {
        if (event == "Command") {
            serialCommandManager.processCommand(params[0]);
        } else if (event == "Reboot") {
            eventManager.debug("Restarting (event)...", 1);
            ESP.restart();
        } else {
            processUI(event, params);
        }
#endif
    } else if (type == "serial") {
        if (event == "input") {
            processInput(params[0]);
        }
        if (event == "command") {
            eventManager.triggerEvent("espui", "SerialIn", params);
        }
    } else if (type == "telnet") {
        if (event == "input") {
            processInput(params[0]);
        }
    }
}

bool MainController::processInput(const String input)
{
    eventManager.debug("Processing input: " + input, 2);
    if (input.length() == 0) {
        eventManager.debug("Empty input", 1);
        return false;
    }
    int nsIndex = input.indexOf(':');
    int cmdIndex = input.indexOf(' ');

    if (nsIndex == -1 || (cmdIndex != -1 && cmdIndex < nsIndex)) {
        eventManager.debug("Erreur: Format de commande incorrect: " + input, 0);
        return false;
    }

    // Extraction du namespace et de la commande
    String ns = input.substring(0, nsIndex);
    String command = cmdIndex == -1 ? input.substring(nsIndex + 1) : input.substring(nsIndex + 1, cmdIndex);

    // Extraction des paramètres
    String paramStr = cmdIndex == -1 ? "" : input.substring(cmdIndex + 1);
    std::vector<String> params = paramStr.isEmpty() ? std::vector<String>() : splitParameters(paramStr);

    //eventManager.triggerEvent("serial", "command", {input});
    eventManager.triggerEvent(ns, "@" + command, params);
    return true;
}

void MainController::processCommand(String command, std::vector<String> params)
{
    /*for (auto &device : devices)
    {
        device->processCommand(command, params);
    }*/
    // if command is a digit
    if (command.length() == 1 && isdigit(command[0])) {
        params.insert(params.begin(), String(command[0]));
        command = "debuglevel";
    }

    if (command == "ping") {
        Serial.println("Pong");
    } else if (command == "info") {
        eventManager.debug("Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz", 0);
        eventManager.debug("Flash size: " + String(ESP.getFlashChipSize() / 1024) + " KB", 0);
        eventManager.debug("Free heap: " + String(ESP.getFreeHeap()), 0);
        eventManager.debug("Sketch size: " + String(ESP.getSketchSize()), 0);
        eventManager.debug("Free sketch space: " + String(ESP.getFreeSketchSpace()), 0);
#ifdef ESP32
        eventManager.debug("Chip ID: " + String(ESP.getEfuseMac()), 0);
        eventManager.debug("Chip model: " + String(ESP.getChipModel()), 0);
        eventManager.debug("Chip revision: " + String(ESP.getChipRevision()), 0);
        eventManager.debug("Chip core: " + String(ESP.getChipCores()), 0);
#endif
        eventManager.debug("Reset reason: " + ESP.getResetReason(), 0);
        eventManager.debug("Hostname: " + config.getHostname(), 0);
        eventManager.debug("Debug level: " + String(config.getPreference("debug_level", 0)), 0);
        eventManager.debug("Power saving: " + String(wifi_get_sleep_type() == NONE_SLEEP_T ? "disabled" : "enabled"), 0);
        eventManager.debug("Power saving time: " + String(powerSaving), 0);
        eventManager.debug("Time: " + timeManager.getFormattedDateTime("%d/%m/%Y %H:%M:%S"), 0);
        if (wiFiManager.isConnected()) {
            eventManager.debug("Connected to WiFi: " + wiFiManager.retrieveSSID(), 0);
            eventManager.debug("IP address: " + wiFiManager.retrieveIP(), 0);
        } else {
            eventManager.debug("Not connected to WiFi", 0);
        }
    } else if (command == "fs") {
        if (!LittleFS.begin()) {
            eventManager.debug("Failed to initialize LittleFS", 0);
            return;
        }
        FSInfo fs_info;
        LittleFS.info(fs_info);
        eventManager.debug("Total bytes: " + String(fs_info.totalBytes), 0);
        eventManager.debug("Used bytes: " + String(fs_info.usedBytes), 0);
        eventManager.debug("Free bytes: " + String(fs_info.totalBytes - fs_info.usedBytes), 0);
    } else if (command == "date") {
        eventManager.debug(timeManager.getFormattedDateTime("%d/%m/%Y"), 0);
    } else if (command == "ota") {
        wiFiManager.otaUpdate();
    } else if (command == "restart" || command == "reboot") {
        eventManager.debug("Restarting (command)...", 1);
        ESP.restart();
    } else if (command == "debuglevel") {
        if (params.size() > 0) {
            config.setPreference("debug_level", params[0].toInt());
            eventManager.debug("Debug level set to: " + params[0], 1);
        } else {
            int debugLevel = config.getPreference("debug_level", 0);
            eventManager.debug("Debug level: " + String(debugLevel), 0);
        }
    } else if (command == "power_saving") {
        if (params.size() > 0) {
            if (!isInteger(params[0])) {
                eventManager.debug("Invalid value: " + params[0], 0);
                eventManager.debug("Usage: sys:power_saving <value>, the value should be a number >= 0 (ms) / 0 = disable", 0);
                return;
            }
            setPowerSaving(params[0].toInt());
        } else {
            int powerSaving = config.getPreference("power_saving", 0);
            eventManager.debug("Power saving: " + String(powerSaving), 0);
        }
    } else if (command == "hostname") {
        if (params.size() > 0) {
            config.setPreference("hostname", params[0]);
            eventManager.debug("Hostname set to: " + params[0], 1);
        } else {
            eventManager.debug("Hostname: " + config.getHostname(), 0);
        }
    } else if (command == "config") {
        if (params.size() > 0) {
            config.setJsonConfig(params[0]);
            eventManager.debug("Configuration updated", 1);
        } else {
            eventManager.debug(config.getJsonConfig(), 0);
        }
    } else if (command == "ntp") {
        if (timeManager.update(true)) {
            eventManager.debug("Time updated", 0);
            eventManager.debug("Time: " + timeManager.getFormattedDateTime("%d/%m/%Y %H:%M:%S"), 0);
        } else {
            eventManager.debug("Failed to update time", 0);
        }
    } else if (command == "device") {
        if (params.size() == 0) {
            eventManager.debug("List of devices:", 1);
            for (auto& device : devices) {
                eventManager.debug(" #" + device->id + " : " + device->name + " (" + device->topic + ")", 0);
            }
        } else {
            Device* device = getDeviceById(params[0]);
            if (device != nullptr) {
                eventManager.debug("ID: " + device->id, 0);
                eventManager.debug("Type: " + device->type, 0);
                eventManager.debug("Name: " + device->name, 0);
                eventManager.debug("Topic: " + device->topic, 0);
            } else {
                eventManager.debug("Device not found: " + params[0], 0);
            }
        }
    } else {
        eventManager.debug("Unknown command: " + command, 0);
    }
}

void MainController::processMQTT(String topic, String value)
{
    eventManager.debug("Received MQTT message: " + topic + " = " + value, 2);
}

EventManager* MainController::getEventManager()
{
    return &eventManager;
}

void MainController::addDevice(Device* device)
{
    devices.push_back(device);
}

std::vector<Device*> MainController::getDevices()
{
    return devices;
}

Device* MainController::getDeviceById(const String& id) const
{
    auto it = std::find_if(devices.begin(), devices.end(), [&id](const Device* device) { return device->id == id; });
    if (it != devices.end()) {
        return *it;
    }
    return nullptr;
}

Device* MainController::getDeviceByName(const String& name) const
{
    auto it = std::find_if(devices.begin(), devices.end(), [&name](const Device* device) { return device->name == name; });
    if (it != devices.end()) {
        return *it;
    }
    return nullptr;
}

Device* MainController::getDeviceByTopic(const String& topic) const
{
    auto it = std::find_if(devices.begin(), devices.end(), [&topic](const Device* device) { return device->topic == topic; });
    if (it != devices.end()) {
        return *it;
    }
    return nullptr;
}

void MainController::processDebugMessage(String message, int level, bool displayTime)
{
    if (level <= config.getPreference("debug_level", 0)) {
        String logJson;
        if (displayTime && level > 0) {
            String time = timeManager.getFormattedDateTime("%H:%M:%S");
            logJson = "{\"time\":\"" + time + "\",\"level\":" + String(level) + ",\"message\":\"" + message + "\"}";
            message = time + "> " + message;
        } else {
            logJson = "{\"level\":" + String(level) + ",\"message\":\"" + message + "\"}";
        }
        Serial.println(message);
        wiFiManager.printTelnet(message + "\n");
#ifndef DISABLE_ESPUI
        espUIManager.addDebugMessage(message, level);
#endif
        if (mqttManager.isConnected()) {
            mqttManager.publish(config.getHostname() + "/log", logJson, false);
        }
    }
}

void MainController::setPowerSaving(int value, bool save)
{
    if (value < 0) {
        value = config.getPreference("power_saving", 0);
    }
    if (value == 1) {
        value = 100;  // default value
    }
    powerSaving = value;
    if (powerSaving > 0) {
        eventManager.debug("Power saving enabled: process every " + String(powerSaving) + "ms", 1);
        wifi_set_sleep_type(LIGHT_SLEEP_T);  // power saving when idle (delay 100ms in loop)
    } else {
        eventManager.debug("Power saving disabled", 1);
        wifi_set_sleep_type(NONE_SLEEP_T);
    }
    if (save) {
        config.setPreference("power_saving", powerSaving);
    }
}
