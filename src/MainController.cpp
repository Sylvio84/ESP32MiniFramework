#include <MainController.h>
#include <Managers/CommandManager.h>
#include <Managers/TimeManager.h>

MainController::MainController()
    : context(),
      eventManager(),
      configManager(context),
      systemManager(context),
      serialManager(context),
      commandManager(context),
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
    context.registerManager(&serialManager);
    context.registerManager(&commandManager);
    context.registerManager(&wiFiManager);
    context.registerManager(&mqttManager);
    context.registerManager(&timeManager);
    context.registerManager(&deviceManager);
    context.registerManager(&deviceProgramManager);
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

    if (deviceProgramManager.loadDevicePrograms()) {
        eventManager.debug("Device programs loaded successfully", 1);
    } else {
        eventManager.debug("Failed to load device programs", 0);
    }

    // Register TimeManager commands after all managers are initialized
    auto* timeMgr = static_cast<TimeManager*>(context.getManager("TimeManager"));
    if (timeMgr) {
        timeMgr->registerCommands();
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

    String processedInput = input;
    
    // Special case: single digit shortcuts for debug level (0-9)
    if (input.length() == 1 && input[0] >= '0' && input[0] <= '9') {
        processedInput = "sys:debug " + input;
        eventManager.debug("Shortcut: " + input + " -> " + processedInput, 2);
    }

    // Use CommandManager to execute commands
    auto* cmdMgr = static_cast<CommandManager*>(context.getManager("CommandManager"));
    if (cmdMgr) {
        // Execute the command string directly (this also adds to history)
        String result = cmdMgr->executeCommandString(processedInput, CommandSource::Serial);
        if (!result.isEmpty()) {
            serialManager.output(result);
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

// processCommand removed - now handled by CommandManager
// Numeric shortcuts (0-3) are now handled in processInput()

void MainController::processMQTT(String topic, String value)
{
    eventManager.debug("Received MQTT message: " + topic + " = " + value, 2);

    // IMPORTANT: Always forward MQTT messages to devices first
    // This allows devices to handle their own topics (e.g., esp32test/led)
    deviceManager.processEventDevices("mqtt", "message", {topic, value});

    String hostname = configManager.getHostname();
    
    // Check for command topic format: hostname/cmd/...
    if (topic.startsWith(hostname + "/cmd/")) {
        String commandPart = topic.substring(hostname.length() + 5);  // 5 = length of "/cmd/"
        
        // Try CommandManager first
        auto* cmdMgr = static_cast<CommandManager*>(context.getManager("CommandManager"));
        if (cmdMgr) {
            // Build command string from topic and value
            // Convert first / to : for namespace:command format, rest to spaces
            String commandStr = commandPart;
            int firstSlash = commandStr.indexOf('/');
            if (firstSlash > 0) {
                // Replace first / with : to get namespace:command format
                commandStr = commandStr.substring(0, firstSlash) + ":" + commandStr.substring(firstSlash + 1);
            }
            // Replace any remaining / with space for additional arguments
            commandStr.replace('/', ' ');
            
            if (!value.isEmpty()) {
                commandStr += " " + value;
            }
            
            String result = cmdMgr->executeCommandString(commandStr, CommandSource::MQTT);
            if (!result.startsWith("Command not found")) {
                // Publish result back via MQTT on /log topic
                // (device commands handle their own response topics)
                eventManager.triggerEvent("mqtt", "publishAsap", 
                    {hostname + "/log", result});
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
    if (level <= systemManager.getDebugLevel()) {
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
        // Debug messages are not sent via MQTT anymore
        // Only command responses are sent to MQTT
        /*if (mqttManager.isConnected()) {
            mqttManager.publish(configManager.getHostname() + "/log", logJson, false);
        }*/
    }
}


