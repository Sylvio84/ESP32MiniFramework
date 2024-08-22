#ifndef MQTTMANAGER_H
#define MQTTMANAGER_H

#include <Arduino.h>
#include <vector>
#include <PubSubClient.h>
// #include <WiFiManager.h>
#include <Configuration.h>
#include <EventManager.h>
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

class MQTTManager
{

public:
    MQTTManager(Configuration& config, EventManager &eventMgr) : config(config), mqttClient(wifiClient)
    {
        this->eventManager = &eventMgr;
        this->hostname = config.HOSTNAME;
    }

    void init();
    void loop();
    bool reconnect();

    bool isConnected();

    // void onMessage(char* topic, byte* payload, unsigned int length);

    // void registerCallback(MQTTCallback callback);
    void publish(String topic, String payload);
    void subscribe(String topic);
    void unsubscribe(String topic);

    void saveServer(String server);
    void savePort(int port);
    void saveUsername(String username);
    void savePassword(String password);

    String retrieveServer();
    int retrievePort();
    String retrieveUsername();
    String retrievePassword();

    String getDebugInfos();

private:
    Configuration& config;

    const char *hostname;

    WiFiClient wifiClient;
    PubSubClient mqttClient;
    static EventManager *eventManager; // Pointeur vers EventManager

    bool connected = false;

    String username = "";
    String password = "";
    String server = "";
    int port = 1883;
};

#endif
