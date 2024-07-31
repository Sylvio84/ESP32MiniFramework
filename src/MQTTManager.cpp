#include "../include/MQTTManager.h"

EventManager* MQTTManager::eventManager = nullptr;

void MQTTManager::init() {
    prefs.begin("mqtt", false);
    this->server = prefs.getString("server", "");
    this->port = prefs.getInt("port", 1883);
    this->username = prefs.getString("username", "");
    this->password = prefs.getString("password", "");

    mqttClient.setServer(server.c_str(), port);
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        eventManager->triggerEvent("MQTT", "Message", {topic, String((char*)payload, length)});
    });
}

void MQTTManager::loop() {
    if (!mqttClient.connected()) {
        reconnect();
    }
    mqttClient.loop();
}

bool MQTTManager::isConnected() {
    return mqttClient.connected();
}

void MQTTManager::reconnect() {
    if (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection... ");
        if (mqttClient.connect(hostname, username.c_str(), password.c_str())) {
            Serial.println("connected");
            eventManager->triggerEvent("MQTT", "Connected", {this->server});
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 1 seconds");
            delay(1000);
        }
    }
}

void MQTTManager::publish(String topic, String payload) {
    Serial.println("Publishing to " + topic + ": " + payload);
    mqttClient.publish(topic.c_str(), payload.c_str());
}

void MQTTManager::subscribe(String topic) {
    Serial.println("Subscribing to " + topic);
    mqttClient.subscribe(topic.c_str());
}

void MQTTManager::unsubscribe(String topic) {
    Serial.println("Unsubscribing from " + topic);
    mqttClient.unsubscribe(topic.c_str());
}

void MQTTManager::saveServer(String server) {
    this->server = server;
    Serial.println("New MQTT server: " + server);
    prefs.putString("server", server);
}

void MQTTManager::savePort(int port) {
    this->port = port;
    Serial.println("New MQTT port: " + port);
    prefs.putInt("port", port);
}

void MQTTManager::saveUsername(String username) {
    this->username = username;
    Serial.println("New MQTT username: " + username);
    prefs.putString("username", username);
}

void MQTTManager::savePassword(String password) {
    this->password = password;
    Serial.println("New MQTT password: " + password);
    prefs.putString("password", password);
}

String MQTTManager::getDebugInfos() {
    return "Server: " + prefs.getString("server", "") + "\nPort: " + prefs.getInt("port", 0) + "\nUsername: " + prefs.getString("username", "") + "\nPassword: " + prefs.getString("password", "");
}