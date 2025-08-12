#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include <Managers/ConfigurationManager.h>
#include <Managers/EventManager.h>
#include <Managers/TimeManager.h>
#include <FrameworkContext.h>
#include <functional>
#include <DeviceProgram.h>
#include <map>
#include <vector>
#include <Managers/CommandManager.h>
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


    // Méthodes virtuelles pures à implémenter dans les classes dérivées
    virtual void init();
    virtual void loop();

    virtual bool subscribeMQTT(String topic);
    virtual bool unsubscribeMQTT(String topic);

    virtual void processEvent(String type, String event, std::vector<String> params);
    bool processMQTT(String topic, String value);  // Now non-virtual (Template Method Pattern)
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
    
    // Command registration helpers
    void registerDeviceCommand(const String& commandName, 
                              const String& description,
                              std::function<String(const std::vector<String>&)> handler,
                              CommandSource source = CommandSource::Any);
    
    void registerDeviceCommands();
    std::vector<Command> getDeviceCommands() const;
    

#ifndef DISABLE_ESPUI
    void initEspUI();
    void EspUiCallback(Control* sender, int type);
#endif

  protected:
    FrameworkContext* context;
    std::vector<Command> deviceCommands;
    
    /**
     * @brief Device-specific MQTT processing (override in derived classes)
     * @param topic MQTT topic
     * @param value MQTT payload
     * @return true if message was handled, false otherwise
     */
    virtual bool processMQTTDevice(String topic, String value) { return false; }
    
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
