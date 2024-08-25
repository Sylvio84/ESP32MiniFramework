#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include <Configuration.h>
#include <EventManager.h>
#include <ESPUI.h>
#include <functional>
#include <map>

class Device
{
   public:
    // Attributs publics
    String id;
    String name;
    String topic;
    int state;
    String type;

    // Constructeur virtuel
    Device(String id, Configuration& config, EventManager& eventMgr) : config(config)
    {
        this->id = id;
        if (eventManager == nullptr) {
            eventManager = &eventMgr;
        }
    };

    // Méthode pour ajouter une commande et son action associée
    void addCommand(const std::string& command, std::function<void()> action);

    // Méthode pour traiter une commande reçue
    void handleCommand(const std::string& command);

    // Méthodes virtuelles pures à implémenter dans les classes dérivées
    virtual void init();
    virtual void loop() = 0;

    virtual bool subscribeMQTT(String topic);
    virtual bool unsubscribeMQTT(String topic);

    virtual void processEvent(String type, String event, std::vector<String> params);
    virtual bool processMQTT(String topic, String value);
    virtual bool processCommand(String command, std::vector<String> params);
    virtual bool processUI(String action, std::vector<String> params);

    void saveTopic(String topic);
    String retrieveTopic();

    void saveName(String name);
    String retrieveName();

    void initEspUI();
    void EspUiCallback(Control* sender, int type);

   protected:
    Configuration& config;

    // Carte des commandes et de leurs actions associées
    std::map<std::string, std::function<void()>> commands;

    static EventManager* eventManager;  // Pointeur vers EventManager

    // ESPUI:
    uint16_t nameInput = 0;
    uint16_t topicInput = 0;
};

#endif  // DEVICE_H