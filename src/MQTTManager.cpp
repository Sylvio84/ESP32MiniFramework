#include "../include/MQTTManager.h"

EventManager* MQTTManager::eventManager = nullptr;

void MQTTManager::init()
{
    Serial.println("MQTTManager init...");

    retrieveServer();
    retrievePort();
    retrieveUsername();
    retrievePassword();

    if (server == "") {
        Serial.println("No MQTT server configured");
        return;
    }
    if (username == "") {
        Serial.println("Warning: No MQTT username configured");
    }
    mqttClient.setServer(server.c_str(), port);

    mqttClient.setKeepAlive(5);
    mqttClient.setSocketTimeout(100);

    /*mqttClient.setCallback([this](char *topic, byte *payload, unsigned int length)
                           { eventManager->triggerEvent("mqtt", "message", {topic, String((char *)payload, length)}); });*/
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        // Créer une instance de String avec la bonne longueur
        String payloadString;
        payloadString.reserve(length);  // Réserver de l'espace pour éviter les reallocations
        for (unsigned int i = 0; i < length; ++i) {
            payloadString += (char)payload[i];
        }
        eventManager->triggerEvent("mqtt", "message", {topic, payloadString});
    });
}

void MQTTManager::loop()
{
    static unsigned long lastMillis = 0;
    unsigned long currentMillis = millis();

    /*if (!wifiClient.available()) {
        Serial.println("No WiFi connection, MQTT disabled");
        return;
    }*/

    if (currentMillis - lastMillis >= 1000) {
        if ((status >= 2) && !mqttClient.connected()) {
            if (!reconnect()) {
                Serial.println(" try again in 1 seconds");
            }
        }
        lastMillis = currentMillis;
    }
    mqttClient.loop();
}

void MQTTManager::setStatus(uint status)
{
    this->status = status;
}

bool MQTTManager::isConnected()
{
    return mqttClient.connected();
}

bool MQTTManager::reconnect()
{
    if (!mqttClient.connected()) {
        eventManager->triggerEvent("mqtt", "ConnectionInProgress", {});
        eventManager->debug("Attempting MQTT connection...", 1);
        if (mqttClient.connect(hostname, username.c_str(), password.c_str())) {
            eventManager->triggerEvent("mqtt", "Connected", {this->server});
            eventManager->debug("MQTT connected", 1);
            eventManager->debug("Process subscriptions: " + String(subscriptions.size()), 3);
            for (const auto& topic : subscriptions) {
                eventManager->debug("Process subscription " + topic, 3);
                subscribe(topic);
            }
            return true;
        } else {
            eventManager->triggerEvent("mqtt", "ConnectionFailed", {});
            eventManager->debug("MQTT connection failed, rc=" + String(mqttClient.state()), 1);
            return false;
        }
    }
    return true;
}

void MQTTManager::publish(String topic, String payload)
{
    eventManager->debug("Publishing to " + topic + ": " + payload, 2);
    mqttClient.publish(topic.c_str(), payload.c_str());
}

void MQTTManager::subscribe(String topic)
{
    eventManager->debug("Subscribing to " + topic, 2);
    mqttClient.subscribe(topic.c_str());
}

void MQTTManager::unsubscribe(String topic)
{
    eventManager->debug("Unsubscribing from " + topic, 2);
    mqttClient.unsubscribe(topic.c_str());
}

void MQTTManager::saveServer(String server)
{
    this->server = server;
    eventManager->debug("Saving MQTT server: " + server, 1);
    config.setPreference("mq_serv", server);
}

void MQTTManager::savePort(int port)
{
    this->port = port;
    eventManager->debug("Saving MQTT port: " + String(port), 1);
    config.setPreference("mq_port", port);
}

void MQTTManager::saveUsername(String username)
{
    this->username = username;
    eventManager->debug("Saving MQTT username: " + username, 1);
    config.setPreference("mq_user", username);
}

void MQTTManager::savePassword(String password)
{
    this->password = password;
    eventManager->debug("Saving MQTT password: " + password, 1);
    config.setPreference("mq_pass", password);
}

String MQTTManager::retrieveServer()
{
    server = config.getPreference("mq_serv", server);
    return server;
}

int MQTTManager::retrievePort()
{
    port = config.getPreference("mq_port", port);
    return port;
}

String MQTTManager::retrieveUsername()
{
    username = config.getPreference("mq_user", this->username);
    return username;
}

String MQTTManager::retrievePassword()
{
    password = config.getPreference("mq_pass", this->password);
    return password;
}

String MQTTManager::getDebugInfos()
{
    return "Server: " + retrieveServer() + "\nPort: " + retrievePort() + "\nUsername: " + retrieveUsername() + "\nPassword: " + retrievePassword();
}

void MQTTManager::processEvent(String type, String event, std::vector<String> params)
{
    eventManager->debug("Processing MQTT event: " + type + " / " + event, 2);
    for (const auto& param : params) {
        eventManager->debug("Param: " + param, 3);
    }
    if (type == "mqtt") {
        if (event.startsWith("@")) {
            processCommand(event.substring(1), params);
        }
        if (event == "subscribe") {
            eventManager->debug("process event subscribe to " + params[0], 3);
            if (params.size() > 0) {
                addSubscription(params[0]);
                subscribe(params[0]);
            } else {
                eventManager->debug("Missing topic", 1);
            }
        } else if (event == "unsubscribe") {
            if (params.size() > 0) {
                removeSubscription(params[0]);
                unsubscribe(params[0]);
            } else {
                eventManager->debug("Missing topic", 1);
            }
        } else if (event == "publish") {
            if (params.size() > 1) {
                publish(params[0], params[1]);
            } else {
                eventManager->debug("Missing topic or payload", 1);
            }
        }
    }
}

bool MQTTManager::processCommand(String command, std::vector<String> params)
{
    eventManager->debug("Processing MQTT command: " + command, 2);
    if (command == "server") {
        if (params.size() > 0) {
            saveServer(params[1]);
        } else {
            Serial.println("Server: " + retrieveServer());
        }
    } else if (command == "port") {
        if (params.size() > 0) {
            savePort(params[1].toInt());
        } else {
            Serial.println("Port: " + String(retrievePort()));
        }
    } else if (command == "user") {
        if (params.size() > 0) {
            saveUsername(params[1]);
        } else {
            Serial.println("Username: " + retrieveUsername());
        }
    } else if (command == "pass") {
        if (params.size() > 0) {
            savePassword(params[1]);
        } else {
            Serial.println("Password: " + retrievePassword());
        }
    } else if (command == "status") {
        Serial.println("Connected: " + String(isConnected()));
    } else if (command == "connect") {
        reconnect();
    } else if (command == "subscribe") {
        if (params.size() > 0) {
            addSubscription(params[1]);
            subscribe(params[1]);
        } else {
            Serial.println("Missing topic");
        }
    } else if (command == "unsubscribe") {
        if (params.size() > 0) {
            removeSubscription(params[1]);
            unsubscribe(params[1]);
        } else {
            Serial.println("Missing topic");
        }
    } else if (command == "publish") {
        if (params.size() > 1) {
            publish(params[1], params[2]);
        } else {
            Serial.println("Missing topic or payload");
        }
    } else if (command == "subscriptions") {
        for (const auto& topic : getSubscriptions()) {
            Serial.println(topic);
        }
    } else if (command == "debug") {
        Serial.println(getDebugInfos());
    } else {
        return false;
    }
    return true;
}

bool MQTTManager::addSubscription(String topic)
{
    if (topic.length() == 0) {
        return false;
    }
    if (std::find(subscriptions.begin(), subscriptions.end(), topic) == subscriptions.end()) {
        subscriptions.push_back(topic);
        return true;
    }
    return false;
}

bool MQTTManager::removeSubscription(String topic)
{
    if (topic.length() == 0) {
        return false;
    }
    auto it = std::find(subscriptions.begin(), subscriptions.end(), topic);
    if (it != subscriptions.end()) {
        eventManager->debug("Removing subscription: " + topic, 3);
        subscriptions.erase(it);
        return true;
    }
    return false;
}

std::vector<String> MQTTManager::getSubscriptions()
{
    return subscriptions;
}

void MQTTManager::initEspUI()
{
    eventManager->debug("Init MQTTManager ESPUI", 2);

    auto callback = std::bind(&MQTTManager::EspUiCallback, this, std::placeholders::_1, std::placeholders::_2);

    auto mqttTab = ESPUI.addControl(Tab, "", "MQTT");
    mqttServerInput = ESPUI.addControl(Text, "Server", server, Peterriver, mqttTab, callback);
    mqttPortInput = ESPUI.addControl(Number, "Port", String(port), Peterriver, mqttTab, callback);
    mqttUserInput = ESPUI.addControl(Text, "User", username, Peterriver, mqttTab, callback);
    mqttPasswordInput = ESPUI.addControl(Text, "Password", "", Peterriver, mqttTab, callback);

    ESPUI.setInputType(mqttPasswordInput, "password");

    auto mqttSave = ESPUI.addControl(Button, "Save", "Save", Peterriver, mqttTab, callback);

    ESPUI.setEnabled(mqttSave, true);

    auto mqttReconnect = ESPUI.addControl(Button, "Reconnect", "MQTTReconnect", Peterriver, mqttTab, callback);
    ESPUI.setEnabled(mqttReconnect, true);
}

void MQTTManager::EspUiCallback(Control* sender, int type)
{
    eventManager->debug(
        "MQTT ESPUI callback: sender.value = " + sender->value + " sender.id = " + sender->id + " sender.type = " + sender->type + "  / type = " + String(type),
        2);
    if (type == B_DOWN) {
        return;
    }
    if (sender->value == "MQTTSave") {
        std::vector<String> params1;
        params1.push_back(ESPUI.getControl(mqttServerInput)->value);
        eventManager->triggerEvent("ESPUI", "MQTTSaveServer", params1);

        std::vector<String> params2;
        params2.push_back(ESPUI.getControl(mqttPortInput)->value);
        eventManager->triggerEvent("ESPUI", "MQTTSavePort", params2);

        std::vector<String> params3;
        params3.push_back(ESPUI.getControl(mqttUserInput)->value);
        eventManager->triggerEvent("ESPUI", "MQTTSaveUser", params3);

        String password = ESPUI.getControl(mqttPasswordInput)->value;
        if (password.length() > 0) {
            std::vector<String> params4;
            params4.push_back(password);
            eventManager->triggerEvent("ESPUI", "MQTTSavePassword", params4);
        }
    } else if (sender->value == "MQTTReconnect") {
        eventManager->triggerEvent("ESPUI", "MQTTReconnect", {});
    }
}
