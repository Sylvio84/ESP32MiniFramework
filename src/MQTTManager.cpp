#include "../include/MQTTManager.h"

EventManager *MQTTManager::eventManager = nullptr;

void MQTTManager::init()
{
    Serial.println("MQTTManager init...");

    this->server = retrieveServer();
    this->port = retrievePort();
    this->username = retrieveUsername();
    this->password = retrievePassword();

    if (server == "")
    {
        Serial.println("No MQTT server configured");
        return;
    }
    if (username == "")
    {
        Serial.println("Warning: No MQTT username configured");
    }
    mqttClient.setServer(server.c_str(), port);

    mqttClient.setKeepAlive(5);
    mqttClient.setSocketTimeout(100);

    /*mqttClient.setCallback([this](char *topic, byte *payload, unsigned int length)
                           { eventManager->triggerEvent("MQTT", "Message", {topic, String((char *)payload, length)}); });*/
    mqttClient.setCallback([this](char *topic, byte *payload, unsigned int length)
                           {
    // Créer une instance de String avec la bonne longueur
    String payloadString;
    payloadString.reserve(length); // Réserver de l'espace pour éviter les reallocations
    for (unsigned int i = 0; i < length; ++i) {
        payloadString += (char)payload[i];
    }
    eventManager->triggerEvent("MQTT", "Message", {topic, payloadString}); });
}

void MQTTManager::loop()
{
    static unsigned long lastMillis = 0;
    unsigned long currentMillis = millis();

    /*if (!wifiClient.available()) {
        Serial.println("No WiFi connection, MQTT disabled");
        return;
    }*/

    if (currentMillis - lastMillis >= 1000)
    {
        if (!mqttClient.connected())
        {
            if (!reconnect()) {
                Serial.println(" try again in 1 seconds");
            }
        }
    }
    mqttClient.loop();
}

bool MQTTManager::isConnected()
{
    return mqttClient.connected();
}

bool MQTTManager::reconnect()
{
    if (!mqttClient.connected())
    {
        Serial.print("Attempting MQTT connection... ");
        if (mqttClient.connect(hostname, username.c_str(), password.c_str()))
        {
            Serial.println("connected");
            eventManager->triggerEvent("MQTT", "Connected", {this->server});
            return true;
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.println(mqttClient.state());
            return false;
        }
    }
    return true;
}

void MQTTManager::publish(String topic, String payload)
{
    Serial.println("Publishing to " + topic + ": " + payload);
    mqttClient.publish(topic.c_str(), payload.c_str());
}

void MQTTManager::subscribe(String topic)
{
    Serial.println("Subscribing to " + topic);
    mqttClient.subscribe(topic.c_str());
}

void MQTTManager::unsubscribe(String topic)
{
    Serial.println("Unsubscribing from " + topic);
    mqttClient.unsubscribe(topic.c_str());
}

void MQTTManager::saveServer(String server)
{
    this->server = server;
    Serial.println("New MQTT server: " + server);
    config.setPreference("mq_serv", server);
}

void MQTTManager::savePort(int port)
{
    this->port = port;
    Serial.println("New MQTT port: " + port);
    config.setPreference("mq_port", port);
}

void MQTTManager::saveUsername(String username)
{
    this->username = username;
    Serial.println("New MQTT username: " + username);
    config.setPreference("mq_user", username);
}

void MQTTManager::savePassword(String password)
{
    this->password = password;
    Serial.println("New MQTT password: " + password);
    config.setPreference("mq_pass", password);
}

String MQTTManager::retrieveServer()
{
    return config.getPreference("mq_serv", this->server);
}

int MQTTManager::retrievePort()
{
    return config.getPreference("mq_port", this->port);
}

String MQTTManager::retrieveUsername()
{
    return config.getPreference("mq_user", this->username);
}

String MQTTManager::retrievePassword()
{
    return config.getPreference("mq_pass", this->password);
}

String MQTTManager::getDebugInfos()
{
    return "Server: " + retrieveServer() + "\nPort: " + retrievePort() + "\nUsername: " + retrieveUsername() + "\nPassword: " + retrievePassword();
}
