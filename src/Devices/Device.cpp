#include <Devices/Device.h>


void Device::init()
{
    retrieveName();
    retrieveTopic();
#ifndef DISABLE_ESPUI
    initEspUI();
#endif
    subscribeMQTT(topic);

    //addCommand("cmd", std::bind(&Device::executeCmd, this, std::placeholders::_1));

    debug("initialized", 1);
}

void Device::loop() {}


void Device::saveTopic(String topic)
{
    unsubscribeMQTT(this->topic);
    this->topic = topic;
    debug("Saving topic: " + topic, 1);
    static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"))->setPreference(id + "_topic", topic);
    subscribeMQTT(topic);
}

String Device::retrieveTopic()
{
    this->topic = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"))->getPreference(id + "_topic", topic);
    return this->topic;
}

void Device::saveName(const String name)
{
    this->name = name;
    debug("Saving name: " + name, 1);
    static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"))->setPreference(id + "_name", name);
}

String Device::retrieveName()
{
    this->name = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"))->getPreference(id + "_name", name);
    return this->name;
}

void Device::publishName()
{
    debug("Device name: " + name, 0);
    context->getEventManager()->triggerEvent("mqtt", "publishAsap", {topic + "/log", name});
}

bool Device::subscribeMQTT(String topic)
{
    if (topic == "") {
        return false;
    }
    context->getEventManager()->triggerEvent("mqtt", "subscribe", {topic});
    //context->getService<EventManager>()->triggerEvent("mqtt", "subscribe", {topic + "/cmd"});
    return true;
}

bool Device::unsubscribeMQTT(String topic)
{
    if (topic == "") {
        return false;
    }
    context->getEventManager()->triggerEvent("mqtt", "unsubscribe", {topic});
    //context->getService<EventManager>()->triggerEvent("mqtt", "unsubscribe", {topic + "/cmd"});
    return true;
}

void Device::processEvent(String type, String event, std::vector<String> params)
{
    debug("Processing event: " + type + " " + event, 3);
    if ((type == id) && (event.startsWith("@"))) {
        processCommand(event.substring(1), params);
    }
    if (type == "mqtt") {
        if (event == "message") {
            processMQTT(params[0], params[1]);
        }
    }
}

bool Device::processCommand(String command, std::vector<String> params)
{
    debug("Processing command: " + command, 3);
    if (command == "name") {
        if (params.size() > 0) {
            saveName(params[0]);
        } else {
            debug("Name: " + retrieveName(), 0);
        }
        return true;
    }
    if (command == "topic") {
        if (params.size() > 0) {
            saveTopic(params[0]);
        } else {
            debug("Topic: " + retrieveTopic(), 0);
        }
        return true;
    }
    return false;
}

bool Device::processMQTT(String topic, String value)
{
    debug("Processing MQTT message: " + topic + " = " + value, 3);
    
    // 1. PRIORITY 1: Let derived class handle first (device-specific processing)
    if (processMQTTDevice(topic, value)) {
        debug("MQTT message handled by device-specific implementation", 3);
        return true;
    }
    
    // 2. PRIORITY 2: Handle namespaced commands (hostname/cmd/deviceId/commandName)
    String hostname = "ESP32";  // Default hostname
    if (auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"))) {
        String configHostname = configMgr->getHostname();
        if (!configHostname.isEmpty()) {
            hostname = configHostname;
        }
    }
    
    String cmdPrefix = hostname + "/cmd/" + id + "/";
    if (topic.startsWith(cmdPrefix)) {
        String commandName = topic.substring(cmdPrefix.length());
        debug("Received MQTT command: " + commandName + " with value: " + value, 2);
        
        auto* cmdMgr = static_cast<CommandManager*>(context->getManager("CommandManager"));
        if (cmdMgr) {
            // Parse value as arguments (space-separated)
            std::vector<String> args;
            if (!value.isEmpty()) {
                int start = 0;
                int end = value.indexOf(' ');
                while (end != -1) {
                    args.push_back(value.substring(start, end));
                    start = end + 1;
                    end = value.indexOf(' ', start);
                }
                args.push_back(value.substring(start));
            }
            
            // Execute the command in device namespace
            String fullCommand = id + ":" + commandName;
            String result = cmdMgr->executeCommand(fullCommand, args, CommandSource::MQTT);
            
            // Publish result to response topic
            String responseTopic = hostname + "/" + id + "/response";
            context->getEventManager()->triggerEvent("mqtt", "publishAsap", {responseTopic, result});
            return true;
        }
    }
    
    // 3. PRIORITY 3: Base class legacy topic handling
    if (topic == this->topic) {
        debug("Topic " + topic + " matched, processing as command: " + value, 3);
        return processCommand(value, {});
    }
    
    if (topic == this->topic + "/cmd") {
        if (value == "name") {
            context->getEventManager()->triggerEvent("mqtt", "publishAsap", {this->topic + "/log", this->name});
        }
        return true;
    }
    
    // No handler matched
    return false;
}

bool Device::processUI(String action, std::vector<String> params)
{
    return false;
}

#ifndef DISABLE_ESPUI
void Device::initEspUI()
{
    debug("Init ESPUI", 2);

    auto callback = std::bind(&Device::EspUiCallback, this, std::placeholders::_1, std::placeholders::_2);

    auto deviceTab = ESPUI.addControl(Tab, "", name);
    nameInput = ESPUI.addControl(Label, "Name", name, Peterriver, deviceTab, callback);
    topicInput = ESPUI.addControl(Label, "Topic", topic, Peterriver, deviceTab, callback);

    ESPUI.addControl(Button, "Save", "Save", Peterriver, deviceTab, callback);
}

void Device::EspUiCallback(Control* sender, int type)
{
    debug("ESPUI callback: sender.value = " + sender->value + " sender.id = " + sender->id + " sender.type = " + sender->type +
                            "  / type = " + String(type),
                        2);

    if (type == B_DOWN) {
        return;
    }

    if (sender->value == "Save") {
        /*std::vector<String> params1;
        params1.push_back(ESPUI.getControl(nameInput)->value);
        context->getEventManager()->triggerEvent("ESPUI", id + "SaveName", params1);

        std::vector<String> params2;
        params2.push_back(ESPUI.getControl(topicInput)->value);
        context->getEventManager()->triggerEvent("ESPUI", id + "SaveTopic", params2);
        */
        saveName(ESPUI.getControl(nameInput)->value);
        saveTopic(ESPUI.getControl(topicInput)->value);
    }
}
#endif

void Device::onProgramStart()
{
    debug("program started", 1);
}

void Device::onProgramEnd()
{
    debug("program ended", 1);
}

// Simplified debug helper for devices
void Device::debug(const String& message, int level, bool displayTime) {
    if (auto* eventMgr = context->getEventManager()) {
        eventMgr->debug("[Device:" + id + "] " + message, level, displayTime);
    }
}

void Device::registerDeviceCommand(const String& commandName, 
                                  const String& description,
                                  std::function<String(const std::vector<String>&)> handler,
                                  CommandSource source)
{
    auto* cmdMgr = static_cast<CommandManager*>(context->getManager("CommandManager"));
    if (!cmdMgr) {
        debug("CommandManager not available for device commands", 1);
        return;
    }
    
    // Register command with device ID as namespace
    Command cmd(id, commandName, description, source, false, handler);
    deviceCommands.push_back(cmd);
    cmdMgr->registerCommand(cmd);
    
    // Get hostname for MQTT topic mapping
    String hostname = "ESP32";  // Default hostname
    if (auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"))) {
        String configHostname = configMgr->getHostname();
        if (!configHostname.isEmpty()) {
            hostname = configHostname;
        }
    }
    
    // Subscribe to MQTT command topic: hostname/cmd/deviceId/commandName
    String cmdTopic = hostname + "/cmd/" + id + "/" + commandName;
    subscribeMQTT(cmdTopic);
    debug("Registered command " + id + ":" + commandName + " with MQTT topic " + cmdTopic, 2);
}

void Device::registerDeviceCommands()
{
    auto* cmdMgr = static_cast<CommandManager*>(context->getManager("CommandManager"));
    if (!cmdMgr) {
        debug("CommandManager not available for device commands", 1);
        return;
    }
    
    // Register a help command for this device
    registerDeviceCommand("help", "List all commands for this device",
        [this](const std::vector<String>& args) -> String {
            String result = "Commands for device '" + name + "' (" + id + "):\n";
            for (const auto& cmd : deviceCommands) {
                result += "  " + id + ":" + cmd.name + " - " + cmd.description + "\n";
            }
            return result;
        }
    );
    
    // Register default device management commands
    registerDeviceCommand("status", "Get device status",
        [this](const std::vector<String>& args) -> String {
            String result = "Device Status:\n";
            result += "  ID: " + id + "\n";
            result += "  Name: " + name + "\n";
            result += "  Topic: " + topic + "\n";
            result += "  Type: " + type + "\n";
            result += "  State: " + String(state);
            return result;
        }
    );
    
    registerDeviceCommand("setname", "Set device name",
        [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                saveName(args[0]);
                return "Device name set to: " + args[0];
            }
            return "ERROR: Name required";
        }
    );
    
    registerDeviceCommand("settopic", "Set device MQTT topic",
        [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                saveTopic(args[0]);
                return "Device topic set to: " + args[0];
            }
            return "ERROR: Topic required";
        }
    );
}

std::vector<Command> Device::getDeviceCommands() const
{
    return deviceCommands;
}
