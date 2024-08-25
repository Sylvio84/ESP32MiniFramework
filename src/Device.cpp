#include "../include/Device.h"

EventManager* Device::eventManager = nullptr;

void Device::init()
{
    retrieveName();
    retrieveTopic();
    initEspUI();
    subscribeMQTT(topic);
    eventManager->debug("Device #" + id + " initialized", 1);
}

void Device::addCommand(const std::string& command, std::function<void()> action)
{
    commands[command] = action;
}

// Méthode pour traiter une commande reçue
void Device::handleCommand(const std::string& command)
{
    if (commands.find(command) != commands.end()) {
        commands[command]();  // Appelle la fonction associée
    }
}

void Device::saveTopic(String topic)
{
    unsubscribeMQTT(this->topic);
    this->topic = topic;
    Serial.println("Saving topic: " + topic);
    config.setPreference(id + "_topic", topic);
    subscribeMQTT(topic);
}

String Device::retrieveTopic()
{
    this->topic = config.getPreference(id + "_topic", topic);
    return this->topic;
}

void Device::saveName(const String name)
{
    this->name = name;
    Serial.println("Saving name: " + name);
    config.setPreference(id + "_name", name);
}

String Device::retrieveName()
{
    this->name = config.getPreference(id + "_name", name);
    return this->name;
}

bool Device::subscribeMQTT(String topic)
{
    if (topic == "") {
        return false;
    }
    eventManager->triggerEvent("mqtt", "subscribe", {topic});
    return true;
}

bool Device::unsubscribeMQTT(String topic)
{
    if (topic == "") {
        return false;
    }
    eventManager->triggerEvent("mqtt", "unsubscribe", {topic});
    return true;
}

void Device::processEvent(String type, String event, std::vector<String> params)
{
    eventManager->debug("Processing device #" + id + " event: " + type + " " + event, 3);
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
    eventManager->debug("Processing device #" + id + " command: " + command, 3);
    if (command == "name") {
        if (params.size() > 0) {
            saveName(params[0]);
        } else {
            Serial.println("Name: " + retrieveName());
        }
        return true;
    }
    if (command == "topic") {
        if (params.size() > 0) {
            saveTopic(params[0]);
        } else {
            Serial.println("Topic: " + retrieveTopic());
        }
        return true;
    }
    return false;
}

bool Device::processMQTT(String topic, String value)
{
    eventManager->debug("Processing device #" + id + " MQTT message: " + topic + " = " + value, 3);
    eventManager->debug("MyTopic: " + this->topic, 3);
    if (topic == this->topic) {
        eventManager->debug("Topic matched", 3);
        handleCommand(value.c_str());
        return true;
    }
    return false;
}

bool Device::processUI(String action, std::vector<String> params)
{
    return false;
}

void Device::initEspUI()
{
    eventManager->debug("Init " + id + " ESPUI", 2);

    auto callback = std::bind(&Device::EspUiCallback, this, std::placeholders::_1, std::placeholders::_2);

    auto deviceTab = ESPUI.addControl(Tab, "", name);
    nameInput = ESPUI.addControl(Label, "Name", name, Peterriver, deviceTab, callback);
    topicInput = ESPUI.addControl(Label, "Topic", topic, Peterriver, deviceTab, callback);

    ESPUI.addControl(Button, "Save", "Save", Peterriver, deviceTab, callback);
}

void Device::EspUiCallback(Control* sender, int type)
{
    eventManager->debug(id + " ESPUI callback: sender.value = " + sender->value + " sender.id = " + sender->id + " sender.type = " + sender->type +
                            "  / type = " + String(type),
                        2);

    if (type == B_DOWN) {
        return;
    }

    if (sender->value == "Save") {
        /*std::vector<String> params1;
        params1.push_back(ESPUI.getControl(nameInput)->value);
        eventManager->triggerEvent("ESPUI", id + "SaveName", params1);

        std::vector<String> params2;
        params2.push_back(ESPUI.getControl(topicInput)->value);
        eventManager->triggerEvent("ESPUI", id + "SaveTopic", params2);
        */
        saveName(ESPUI.getControl(nameInput)->value);
        saveTopic(ESPUI.getControl(topicInput)->value);
    }
}