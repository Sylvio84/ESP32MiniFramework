#include "../include/MainController.h"

MainController::MainController(Configuration& config) : config(&config),
                                                        displayManager(config),
                                                        wiFiManager(config, eventManager),
                                                        mqttManager(config, eventManager),
                                                        timeManager(config),
                                                        espUIManager(config, eventManager)
{
    //this->config = &config;
    this->hostname = config.HOSTNAME;

    eventManager.registerCallback("ESPUI", [this](String action, std::vector<String> params)
                                  { this->processEvent("ESPUI", action, params); });
    eventManager.registerCallback("WiFi", [this](String action, std::vector<String> params)
                                  { this->processEvent("WiFi", action, params); });
    eventManager.registerCallback("System", [this](String action, std::vector<String> params)
                                  { this->processEvent("System", action, params); });
    eventManager.registerCallback("Serial", [this](String action, std::vector<String> params)
                                  { this->processEvent("Serial", action, params); });
    eventManager.registerCallback("MQTT", [this](String action, std::vector<String> params)
                                  { this->processEvent("MQTT", action, params); });
}

void MainController::init()
{
    Serial.begin(115200);
    Serial.println();
    displayManager.init();
    delay(1000);
    Serial.println("Init...");
    wiFiManager.init();
    timeManager.init();
    mqttManager.init();
    espUIManager.init();
    espUIManager.getControl(espUIManager.ssidInput)->value = wiFiManager.retrieveSSID();

    displayManager.clear();
    if (wiFiManager.isConnected())
    {
        displayManager.printLine(0, "Wifi Connected");
    }
    else
    {
        displayManager.printLine(0, "Not connected");
    }
    Serial.println("Init done!");
    Serial.println("Welcome on " + String(hostname));
}

void MainController::loop()
{
    timeManager.loop();
    mqttManager.loop();
    readSerialCommand();
}

void MainController::readSerialCommand()
{
    if (Serial.available())
    {
        String input = Serial.readStringUntil('\n');
        input.trim();
        int spaceIndex = input.indexOf(' ');
        if (spaceIndex == -1)
        {
            eventManager.triggerEvent("Serial", input, {});
        }
        else
        {
            eventManager.triggerEvent("Serial", input.substring(0, spaceIndex), split(input.substring(spaceIndex + 1), ' '));
        }
    }
}

bool MainController::processCommand(String command)
{
    if (command.indexOf("restart") > -1)
    {
        eventManager.triggerEvent("System", "Reboot", {});
        return true;
    }
    if (command.indexOf("clear") > -1)
    {
        eventManager.triggerEvent("System", "DisplayClear", {});
        return true;
    }
    return false;
}

void MainController::processUI(String action, std::vector<String> params)
{
    // Serial.println("Processing UI: " + action);
    /*for (const auto &param : params)
    {
        Serial.println("Param: " + param);
    }*/

    displayManager.printLine(1, action.c_str());
    if (action == "WiFiConnect")
    {
        wiFiManager.connect();
    }
    else if (action == "WiFiDisconnect")
    {
        wiFiManager.disconnect();
    }
    else if (action == "WiFiHotspot")
    {
        wiFiManager.startAccessPoint();
    }
    else if (action == "WiFiAutoConnect")
    {
        wiFiManager.autoConnect();
    }
    else if (action == "WiFiSaveSSID")
    {
        wiFiManager.saveSSID(params[0]);
    }
    else if (action == "WiFiSavePassword")
    {
        wiFiManager.savePassword(params[0]);
    }
    else if (action == "MQTTSaveServer")
    {
        mqttManager.saveServer(params[0]);
    }
    else if (action == "MQTTSavePort")
    {
        mqttManager.savePort(params[0].toInt());
    }
    else if (action == "MQTTSaveUser")
    {
        mqttManager.saveUsername(params[0]);
    }
    else if (action == "MQTTSavePass")
    {
        mqttManager.savePassword(params[0]);
    }
    else if (action == "MQTTReconnect")
    {
        mqttManager.reconnect();
    }
    else if (action == "Reboot")
    {
        ESP.restart();
    }
    else if (action == "DisplayClear")
    {
        displayManager.clear();
    }
    else if (action == "DisplayPrintLine")
    {
        int line = params[0].toInt();
        displayManager.printLine(line, params[1].c_str());
    }
    else if (action == "WiFiConnected")
    {
        Serial.println("Connected to WiFi: " + params[0]);
        Serial.println("IP address: " + params[1]);
        timeManager.update();
    }
}

void MainController::processCommand(String command, std::vector<String> params)
{
    /* Serial.println("Processing Command: " + command);
    for (const auto &param : params)
    {
        Serial.println("Param: " + param);
    }
    */
    // myESPUI.print(serialLabelId, command);

    if (command == "ssid")
    {
        wiFiManager.saveSSID(params[0]);
        Serial.println("SSID: " + params[0]);
    }
    else if (command == "password")
    {
        wiFiManager.savePassword(params[0]);
        Serial.println("Password: " + params[0]);
    }
    else if (command == "wifi")
    {
        if (params[0] == "connect")
        {
            wiFiManager.connect();
        }
        if (params[0] == "disconnect")
        {
            wiFiManager.disconnect();
        }
        if (params[0] == "hotspot")
        {
            wiFiManager.startAccessPoint();
        }
        if (params[0] == "autoconnect")
        {
            wiFiManager.autoConnect();
        }
        if (params[0] == "status")
        {
            Serial.println("Connected: " + String(wiFiManager.isConnected()));
            Serial.println("SSID: " + wiFiManager.retrieveSSID());
            Serial.println("IP: " + wiFiManager.retrieveIP());
        }
        if (params[0] == "debug")
        {
            Serial.println(wiFiManager.getDebugInfos());
        }
    }
    else if (command == "mqttserver")
    {
        mqttManager.saveServer(params[0]);
    }
    else if (command == "mqttport")
    {
        mqttManager.savePort(params[0].toInt());
    }
    else if (command == "mqttuser")
    {
        mqttManager.saveUsername(params[0]);
    }
    else if (command == "mqttpass")
    {
        mqttManager.savePassword(params[0]);
    }
    else if (command == "mqtt")
    {
        if (params[0] == "status")
        {
            Serial.println("Connected: " + String(mqttManager.isConnected()));
        }
        if (params[0] == "connect")
        {
            mqttManager.reconnect();
        }
        if (params[0] == "subscribe")
        {
            mqttManager.subscribe(params[1]);
        }
        if (params[0] == "unsubscribe")
        {
            mqttManager.unsubscribe(params[1]);
        }
        if (params[0] == "publish")
        {
            mqttManager.publish(params[1], params[2]);
        }
        if (params[0] == "debug")
        {
            Serial.println(mqttManager.getDebugInfos());
        }
    }
    else if (command == "info")
    {
        Serial.println("Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz");
        Serial.println("Flash size: " + String(ESP.getFlashChipSize() / 1024) + " KB");
        Serial.println("Free heap: " + String(ESP.getFreeHeap()));
        Serial.println("Free sketch space: " + String(ESP.getFreeSketchSpace()));
        Serial.println("Chip ID: " + String(ESP.getEfuseMac()));
        Serial.println("Chip model: " + String(ESP.getChipModel()));
        Serial.println("Chip revision: " + String(ESP.getChipRevision()));
        Serial.println("Chip core: " + String(ESP.getChipCores()));
        Serial.println("Hostname: " + String(hostname));

        if (wiFiManager.isConnected())
        {
            Serial.println("Connected to WiFi: " + wiFiManager.retrieveSSID());
            Serial.println("IP address: " + wiFiManager.retrieveIP());
        }
        else
        {
            Serial.println("Not connected to WiFi");
        }
    }
    else if (command == "date")
    {
        Serial.println(timeManager.getFormattedDateTime("%d/%m/%Y %H:%M:%S"));
    }
    else if (command == "restart")
    {
        ESP.restart();
    }
}

void MainController::processEvent(String type, String action, std::vector<String> params)
{
    // Serial.println("Processing Event: " + type + " / " + action);
    /*for (const auto &param : params)
    {
        Serial.println("Param: " + param);
    }*/
    if (type == "WiFi")
    {
        // wiFiManager.processEvent(action, params);
        if (action == "Connected")
        {
            Serial.println("Connected to WiFi: " + params[0]);
            Serial.println("IP address: " + params[1]);
            timeManager.init();
        }
    }
    else if (type == "System")
    {
        if (action == "Reboot")
        {
            ESP.restart();
        }
        else if (action == "DisplayClear")
        {
            displayManager.clear();
        }
    }
    else if (type == "MQTT")
    {
        if (action == "Connected")
        {
            Serial.println("Connected to MQTT server: " + params[0]);
        }
        else if (action == "Message")
        {
            processMQTT(params[0], params[1]);
        }
    }
    else if (type == "Serial")
    {
        processCommand(action, params);
    }
    else if (type == "ESPUI")
    {
        processUI(action, params);
    }
}

void MainController::processMQTT(String topic, String value)
{
    Serial.println("Received MQTT message: " + topic + " = " + value);
}

EventManager *MainController::getEventManager()
{
    return &eventManager;
}
