#ifndef MQTTMANAGER_H
#define MQTTMANAGER_H

#include <Arduino.h>
#include <vector>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <EventManager.h>

class MQTTManager {

public:
    MQTTManager(Configuration config, EventManager &eventMgr): mqttClient(wifiClient) {
        this->eventManager = &eventMgr;
        this->hostname = config.HOSTNAME;
    }

    void init();
    void loop();
    void reconnect();

    bool isConnected();

    //void onMessage(char* topic, byte* payload, unsigned int length);

    //void registerCallback(MQTTCallback callback);
    void publish(String topic, String payload);
    void subscribe(String topic);
    void unsubscribe(String topic);

    void saveServer(String server);
    void savePort(int port);
    void saveUsername(String username);
    void savePassword(String password);

    String getDebugInfos();

private:
    const char *hostname;

    WiFiClient wifiClient;
    PubSubClient mqttClient;
    static EventManager *eventManager; // Pointeur vers EventManager

    Preferences prefs;
    bool connected = false;

    String username = "";
    String password = "";
    String server = "";
    int port = 1883;
};

#endif