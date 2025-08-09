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

void Device::addCommand(const std::string& command, std::function<void()> action)
{
    commands[command] = action;
}


// Méthode pour traiter une commande reçue
bool Device::handleCommand(const std::string& command)
{
    if (commands.find(command) != commands.end()) {
        debug("Command found", 3);
        commands[command]();  // Appelle la fonction associée
        return true;
    }
    debug("Command not found", 3);
    return false;
}

void Device::saveTopic(String topic)
{
    unsubscribeMQTT(this->topic);
    this->topic = topic;
    Serial.println("Saving topic: " + topic);
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
    Serial.println("Saving name: " + name);
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
    debug("MyTopic: " + this->topic, 3);
    if (topic == this->topic) {
        debug("Topic " + topic + " matched, command:" + value, 3);
        return handleCommand(value.c_str());
    }
    if (topic == this->topic + "/cmd") {
        if (value == "name") {
            context->getEventManager()->triggerEvent("mqtt", "publishAsap", {this->topic + "/log", this->name});
        }
    }
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
