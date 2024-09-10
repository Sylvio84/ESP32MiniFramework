#ifndef MQTTMANAGER_H
#define MQTTMANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <vector>
// #include <WiFiManager.h>
#include <Configuration.h>
#ifndef DISABLE_ESPUI
#include <ESPUI.h>
#endif
#include <EventManager.h>
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

class MQTTManager
{

  public:
    MQTTManager(Configuration& config, EventManager& eventMgr) : config(config), mqttClient(wifiClient) { this->eventManager = &eventMgr; }

    // 0 = disabled, 1 = waiting wifi to connect, 2 = keep connected
    uint status = 0;

    String username = "";
    String password = "";
    String server = "";
    int port = 1883;

    void init();
    void loop();

    void processEvent(String type, String event, std::vector<String> params);
    bool processCommand(String action, std::vector<String> params);

    void setStatus(uint status);

    bool reconnect();
    bool isConnected();

    // void onMessage(char* topic, byte* payload, unsigned int length);

    // void registerCallback(MQTTCallback callback);
    void publish(String topic, String payload, bool enableDebug = true);
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

    bool addSubscription(String topic);
    bool removeSubscription(String topic);
    std::vector<String> getSubscriptions();

#ifndef DISABLE_ESPUI
    void initEspUI();
    void EspUiCallback(Control* sender, int type);
#endif

  private:
    Configuration& config;

    WiFiClient wifiClient;
    PubSubClient mqttClient;
    static EventManager* eventManager;  // Pointeur vers EventManager

    bool connected = false;

    std::vector<String> subscriptions;

#ifndef DISABLE_ESPUI
    // ESPUI:
    uint16_t mqttServerInput = 0;
    uint16_t mqttPortInput = 0;
    uint16_t mqttUserInput = 0;
    uint16_t mqttPasswordInput = 0;
#endif
};

#endif
