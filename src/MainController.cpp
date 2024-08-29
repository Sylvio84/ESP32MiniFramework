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
    // this->config = &config;
    this->hostname = config.getPreference("hostname", config.HOSTNAME);

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
    eventManager.debug("Welcome on " + String(hostname));
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
        ESP.restart();
    } else if (action == "DisplayClear") {
        displayManager.clear();
    } else if (action == "DisplayPrintLine") {
        int line = params[0].toInt();
        displayManager.printLine(line, params[1].c_str());
    } else if (action == "WiFiConnected") {
        Serial.println("Connected to WiFi: " + params[0]);
        Serial.println("IP address: " + params[1]);
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
    // displayManager.processEvent(type, event, params);
    for (auto& device : devices) {
        device->processEvent(type, event, params);
    }
    if (type == "wifi") {
        if (event == "connected" || event == "recovered") {
            Serial.println("Connected to WiFi: " + params[0]);
            Serial.println("IP address: " + params[1]);
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
            Serial.println("Connected to MQTT server: " + params[0]);
        } else if (event == "message") {
            processMQTT(params[0], params[1]);
        }
    } else if (type == "espui") {
        processUI(event, params);
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
        Serial.println("Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz");
        Serial.println("Flash size: " + String(ESP.getFlashChipSize() / 1024) + " KB");
        Serial.println("Free heap: " + String(ESP.getFreeHeap()));
        Serial.println("Free sketch space: " + String(ESP.getFreeSketchSpace()));
#ifdef ESP32
        Serial.println("Chip ID: " + String(ESP.getEfuseMac()));
        Serial.println("Chip model: " + String(ESP.getChipModel()));
        Serial.println("Chip revision: " + String(ESP.getChipRevision()));
        Serial.println("Chip core: " + String(ESP.getChipCores()));
#endif
        Serial.println("Hostname: " + String(hostname));
        Serial.println("Debug level: " + String(config.getPreference("debug_level", 0)));
        Serial.println("Time: " + timeManager.getFormattedDateTime("%d/%m/%Y %H:%M:%S"));

        if (wiFiManager.isConnected()) {
            Serial.println("Connected to WiFi: " + wiFiManager.retrieveSSID());
            Serial.println("IP address: " + wiFiManager.retrieveIP());
        } else {
            Serial.println("Not connected to WiFi");
        }
    } else if (command == "date") {
        Serial.println(timeManager.getFormattedDateTime("%d/%m/%Y %H:%M:%S"));
    } else if (command == "restart" || command == "reboot") {
        ESP.restart();
    } else if (command == "debuglevel") {
        if (params.size() > 0) {
            config.setPreference("debug_level", params[0].toInt());
            Serial.println("Debug level set to: " + params[0]);
        } else {
            int debugLevel = config.getPreference("debug_level", 0);
            Serial.println("Debug level: " + String(debugLevel));
        }
    } else if (command == "hostname") {
        if (params.size() > 0) {
            config.setPreference("hostname", params[0]);
            hostname = params[0];
        } else {
            Serial.println("Hostname: " + hostname);
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
        Serial.println("Unknown command: " + command);
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
        if (level > 0) {
            String time = timeManager.getFormattedDateTime("%H:%M:%S");
            Serial.println(time + "> " + message);
        } else {
            Serial.println("> " + message);
        }
    }
}