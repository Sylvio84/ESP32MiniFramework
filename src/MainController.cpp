#include "../include/MainController.h"

MainController::MainController(Configuration& config)
    : eventManager(),
      config(config),
      serialCommandManager(config, eventManager),
      displayManager(config),
      wiFiManager(config, eventManager),
      mqttManager(config, eventManager),
      timeManager(config),
      espUIManager(config, eventManager)
{
    eventManager.registerMainCallback(
        [this](const String& eventType, const String& event, const std::vector<String>& params) { this->processEvent(eventType, event, params); });
    eventManager.registerDebugCallback([this](const String& message, int level) { this->processDebugMessage(message, level); });
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
    espUIManager.init();
    wiFiManager.initEspUI();
    mqttManager.initEspUI();
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
    eventManager.debug("Welcome on " + config.getHostname());
}

void MainController::loop()
{
    serialCommandManager.loop();
    timeManager.loop();
    wiFiManager.loop();
    if (wiFiManager.isConnected()) {
        mqttManager.loop();
    }
    // readSerialCommand();
    for (auto& device : devices) {
        device->loop();
    }
}

void MainController::processUI(String action, std::vector<String> params)

{
    eventManager.debug("Processing UI: " + action, 2);
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

void MainController::processEvent(String type, String event, std::vector<String> params)
{
    eventManager.debug("Processing Event: " + type + " / " + event, 2);
    for (const auto& param : params) {
        eventManager.debug("Param: " + param, 3);
    }

    wiFiManager.processEvent(type, event, params);
    mqttManager.processEvent(type, event, params);
    espUIManager.processEvent(type, event, params);
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
    } else if (type == "sys" && event.startsWith("@")) {
        processCommand(event.substring(1), params);
    } else if (type == "mqtt") {
        if (event == "connected") {
            eventManager.debug("Connected to MQTT server: " + params[0], 1);
        } else if (event == "message") {
            processMQTT(params[0], params[1]);
        }
    } else if (type == "espui") {
        if (event == "Command") {
            serialCommandManager.processCommand(params[0]);
        } else if (event == "Reboot") {
            eventManager.debug("Restarting (event)...", 1);
            ESP.restart();
        } else {
            processUI(event, params);
        }
    } else if (type == "serial") {
        if (event == "command") {
            eventManager.triggerEvent("espui", "SerialIn", params);
        }
    }
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

    if (command == "info") {
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

void MainController::processDebugMessage(String message, int level)
{
    if (level <= config.getPreference("debug_level", 0)) {
        String logJson;
        if (level > 0) {
            String time = timeManager.getFormattedDateTime("%H:%M:%S");
            logJson = "{\"time\":\"" + time + "\",\"level\":" + String(level) + ",\"message\":\"" + message + "\"}";
            message = time + "> " + message;
        } else {
            logJson = "{\"level\":" + String(level) + ",\"message\":\"" + message + "\"}";
        }
        Serial.println(message);
        espUIManager.addDebugMessage(message, level);
        mqttManager.publish(config.getHostname() + "/log", logJson, false);
    }
}
