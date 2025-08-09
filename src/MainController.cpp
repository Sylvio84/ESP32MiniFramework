#include <MainController.h>
#include <CommandManager.h>

MainController::MainController()
    : context(),
      eventManager(),
      configManager(context),
      systemManager(context),
      serialCommandManager(context),
      commandManager(context),
#ifndef DISABLE_DISPLAY
      displayManager(context),
#endif
      wiFiManager(context),
      mqttManager(context),
      timeManager(context),
#ifndef DISABLE_ESPUI
      espUIManager(context),
#endif
      deviceManager(context),
      deviceProgramManager(context)
{
    // Register all services in the context
    context.registerService(&eventManager);
    
    // Register all managers (including ConfigurationManager which is now a Manager)
    context.registerManager(&configManager);
    context.registerManager(&systemManager);
    context.registerManager(&serialCommandManager);
    context.registerManager(&commandManager);
    context.registerManager(&wiFiManager);
    context.registerManager(&mqttManager);
    context.registerManager(&timeManager);
    context.registerManager(&deviceManager);
    context.registerManager(&deviceProgramManager);
#ifndef DISABLE_DISPLAY
    context.registerManager(&displayManager);
#endif
#ifndef DISABLE_ESPUI
    context.registerManager(&espUIManager);
#endif

    eventManager.registerMainCallback(
        [this](const String& eventType, const String& event, const std::vector<String>& params) { this->processEvent(eventType, event, params); });
    eventManager.registerDebugCallback(
        [this](const String& message, int level, bool displayTime = true) { this->processDebugMessage(message, level, displayTime); });
}

void MainController::init()
{
    delay(500);

    // Initialize all managers using clean Manager interface
    for (auto* manager : context.getManagers()) {
        manager->init();
    }
    
    mqttManager.addSubscription(configManager.getHostname() + "/cmd/#");
    //mqttManager.addSubscription(configManager.getHostname() + "/#");
#ifndef DISABLE_ESPUI
    wiFiManager.initEspUI();
    mqttManager.initEspUI();
#endif
    deviceManager.initDevices();

#ifndef DISABLE_DISPLAY
    displayManager.clear();
    if (wiFiManager.isConnected()) {
        displayManager.printLine(0, "Wifi Connected");
    } else {
        displayManager.printLine(0, "Not connected");
    }
#endif

    if (deviceProgramManager.loadDevicePrograms()) {
        eventManager.debug("Device programs loaded successfully", 1);
    } else {
        eventManager.debug("Failed to load device programs", 0);
    }

    // Generate automatic help commands now that all managers have registered their commands
    auto* cmdMgr = static_cast<CommandManager*>(context.getManager("CommandManager"));
    if (cmdMgr) {
        cmdMgr->generateHelpCommands();
    }
    
    eventManager.debug("Init done!", 1);
    eventManager.debug("Welcome on " + configManager.getHostname() + "!", 0);
}

void MainController::loop()
{
    // Call loop on all managers using clean Manager interface
    for (auto* manager : context.getManagers()) {
        manager->loop();
    }
    
    deviceManager.loopDevices();
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

    // Try specialized managers first using clean delegation
    for (auto* manager : context.getManagers()) {
        if (manager->onEvent(type, event, params)) {
            // Event was handled by a manager
            deviceManager.processEventDevices(type, event, params);
            return;
        }
    }
    
    // Handle remaining events that need MainController-specific logic
    if (type == "mqtt" && event == "message") {
        // MQTT messages need special handling in MainController
        processMQTT(params[0], params[1]);
        return;
    }
    
    if ((type == "serial" || type == "telnet") && event == "input") {
        // Input processing needs MainController logic
        processInput(params[0]);
        return;
    }
    
#ifndef DISABLE_ESPUI
    if (type == "espui" && event != "Command" && event != "Reboot") {
        // Custom UI events need MainController processUI
        processUI(event, params);
        return;
    }
#endif

    // Send to device manager for device-specific events
    deviceManager.processEventDevices(type, event, params);
}

bool MainController::processInput(const String input)
{
    eventManager.debug("Processing input: " + input, 2);
    if (input.length() == 0) {
        eventManager.debug("Empty input", 1);
        return false;
    }

    // Use CommandManager to execute commands
    auto* cmdMgr = static_cast<CommandManager*>(context.getManager("CommandManager"));
    if (cmdMgr) {
        // Parse the input to extract command and parameters
        std::vector<String> params;
        String cmdInput = input;
        cmdInput.trim();
        
        // Split by spaces to get command and parameters
        int spaceIndex = cmdInput.indexOf(' ');
        String command;
        if (spaceIndex > 0) {
            command = cmdInput.substring(0, spaceIndex);
            String paramStr = cmdInput.substring(spaceIndex + 1);
            paramStr.trim();
            
            // Split parameters by spaces
            while (paramStr.length() > 0) {
                int nextSpace = paramStr.indexOf(' ');
                if (nextSpace > 0) {
                    params.push_back(paramStr.substring(0, nextSpace));
                    paramStr = paramStr.substring(nextSpace + 1);
                    paramStr.trim();
                } else {
                    params.push_back(paramStr);
                    break;
                }
            }
        } else {
            command = cmdInput;
        }
        
        // Execute the command
        String result = cmdMgr->executeCommand(command, params, CommandSource::Serial);
        if (!result.isEmpty()) {
            Serial.println(result);
        }
        return true;
    }

    // If CommandManager didn't handle it, try MQTT publish format
    int topicIndex = input.indexOf('/');
    if (topicIndex > -1) {
        int spaceIndex = input.indexOf(' ');
        if (spaceIndex > topicIndex) {
            String topic = input.substring(0, spaceIndex);
            eventManager.triggerEvent("mqtt", "publishAsap", {topic, input.substring(spaceIndex + 1)});
            return true;
        }
    }

    eventManager.debug("Command not found: " + input, 0);
    return false;
}

void MainController::processCommand(String command, std::vector<String> params)
{
    // Special case: numeric command becomes debuglevel
    if (command.length() == 1 && isdigit(command[0])) {
        params.insert(params.begin(), String(command[0]));
        command = "debuglevel";
    }
    
    // Try all managers to handle the command using clean Manager interface
    for (auto* manager : context.getManagers()) {
        if (manager->onCommand(command, params)) {
            return; // Command handled
        }
    }
    
    // Command not handled
    eventManager.debug("Unknown command: " + command, 0);
}

void MainController::processMQTT(String topic, String value)
{
    eventManager.debug("Received MQTT message: " + topic + " = " + value, 2);

    String hostname = configManager.getHostname();
    
    // Check for command topic format: hostname/cmd/...
    if (topic.startsWith(hostname + "/cmd/")) {
        String commandPart = topic.substring(hostname.length() + 5);  // 5 = length of "/cmd/"
        
        // Try CommandManager first
        auto* cmdMgr = static_cast<CommandManager*>(context.getManager("CommandManager"));
        if (cmdMgr) {
            // Build command string from topic and value
            String commandStr = commandPart;
            commandStr.replace('/', ' ');  // Replace / with space for command parsing
            if (!value.isEmpty()) {
                commandStr += " " + value;
            }
            
            String result = cmdMgr->executeCommandString(commandStr, CommandSource::MQTT);
            if (!result.startsWith("Command not found")) {
                // Publish result back via MQTT
                eventManager.triggerEvent("mqtt", "publishAsap", 
                    {hostname + "/status/" + commandPart, result});
                return;
            }
        }
        
        // Fallback to old format for backward compatibility
        int slashIndex = commandPart.indexOf('/');
        if (slashIndex > 0) {
            String ns = commandPart.substring(0, slashIndex);
            String command = commandPart.substring(slashIndex + 1);
            
            eventManager.debug("Processing command: " + ns + ":" + command + " with value: " + value, 1);
            
            if (value.startsWith("{") && value.endsWith("}")) {
                // Handle JSON value
                eventManager.triggerEvent(ns, "@" + command, {value});
            } else {
                std::vector<String> params;
                if (value.length() > 0) {
                    params.push_back(value);
                }
                eventManager.triggerEvent(ns, "@" + command, params);
            }
        }
    }
}

EventManager* MainController::getEventManager()
{
    return &eventManager;
}

void MainController::processDebugMessage(String message, int level, bool displayTime)
{
    if (level <= configManager.getPreference("debug_level", 0)) {
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
            mqttManager.publish(configManager.getHostname() + "/log", logJson, false);
        }
    }
}


