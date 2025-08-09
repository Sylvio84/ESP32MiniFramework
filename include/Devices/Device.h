#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include <ConfigurationManager.h>
#include <EventManager.h>
#include <TimeManager.h>
#include <FrameworkContext.h>
#include <functional>
#include <DeviceProgram.h>
#include <map>
#ifndef DISABLE_ESPUI
#include <ESPUI.h>
#endif

class Device
{
  public:
    // Attributs publics
    String id;
    String name;
    String topic;
    int state;
    String type;

    Device& operator=(const Device&) = delete;
    Device& operator=(Device&&) = delete;

    // Constructor with FrameworkContext
    Device(String id, FrameworkContext& ctx) : context(&ctx)
    {
        this->id = id;
    }

    // Méthode pour ajouter une commande et son action associée
    void addCommand(const std::string& command, std::function<void()> action);

    // Méthode pour traiter une commande reçue
    bool handleCommand(const std::string& command);

    // Méthodes virtuelles pures à implémenter dans les classes dérivées
    virtual void init();
    virtual void loop();

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
    void publishName();

    virtual void onProgramStart();
    virtual void onProgramEnd();

    bool importProgram(const String& json);
    //String exportProgram();
    

#ifndef DISABLE_ESPUI
    void initEspUI();
    void EspUiCallback(Control* sender, int type);
#endif

  protected:
    FrameworkContext* context;

    // Carte des commandes et de leurs actions associées
    std::map<std::string, std::function<void()>> commands;
    
    /**
     * @brief Simplified debug helper for devices
     * @param message Debug message
     * @param level Log level (0=info, 1=debug, 2=verbose)
     * @param displayTime Whether to display timestamp
     */
    void debug(const String& message, int level = 0, bool displayTime = true);


    // ESPUI:
    uint16_t nameInput = 0;
    uint16_t topicInput = 0;
};

#endif  // DEVICE_H
