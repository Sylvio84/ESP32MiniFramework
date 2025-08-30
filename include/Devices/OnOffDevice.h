#ifndef ONOFFDEVICE_H
#define ONOFFDEVICE_H

#include <Arduino.h>
#include <Devices/Device.h>
#include <Managers/CommandManager.h>
#include <Command.h>

class OnOffDevice : public Device
{
protected:
    int pin = -1;
    bool state = false;           // État logique du device (ON/OFF)
    bool invertedLogic = false;   // true = LOW pour ON, false = HIGH pour ON
    int timeoutId = 0;
    
    // Méthode interne pour gérer la logique inversée
    void writePin(bool logicalState);
    
public:
    // Constructor
    OnOffDevice(String id, FrameworkContext& ctx);
    
    // Virtual method implementations
    void init() override;
    void loop() override;
    
    // ON/OFF control methods
    virtual void activate(int timeout = 0);
    virtual void deactivate();
    void toggle();
    bool getState();
    void setState(bool newState);  // Compatibility method
    bool isOn();                    // Compatibility method
    void publishState();
    
    // Configuration methods
    virtual void setPin(int newPin);
    void setInvertedLogic(bool inverted);
    
    // Event handlers
    void onProgramStart() override;
    void onProgramEnd() override;
    
    // Command processing
    bool processCommand(String command, std::vector<String> params) override;
    
protected:
    // MQTT processing (Template Method Pattern)
    bool processMQTTDevice(String topic, String value) override;
    
    // Virtual method for device-specific commands registration
    virtual void registerSpecificCommands() = 0;
    
    // Base commands registration
    void registerBaseCommands();
    
    // Helper to check if string is integer
    bool isInteger(const String& str);
};

#endif  // ONOFFDEVICE_H