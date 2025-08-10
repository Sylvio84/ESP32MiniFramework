#ifndef INTERNALLEDDEVICE_H
#define INTERNALLEDDEVICE_H

#include <Arduino.h>
#include <Devices/Device.h>
#include <esp_chip_info.h>
#include <CommandManager.h>
#include <Command.h>

class InternalLedDevice : public Device
{
public:
    InternalLedDevice(String id, FrameworkContext& ctx);
    
    void init() override;
    void loop() override;
    
    // LED control methods
    void activate();
    void deactivate();
    bool getState();
    void toggle();
    void ledOn();
    void ledOff();
    void setState(bool state);
    bool isOn();
    
    // Blink pattern methods
    void startBlinkPattern(int intervalMs, int durationMs);
    void processBlinkPattern();
    
    // Configuration methods
    void setPin(int ledPin);
    void detectLedPin();
    void updateTopicFromConfiguration();
    
    // Command processing
    bool processCommand(String command, std::vector<String> params) override;
    bool processMQTT(String topic, String value) override;
    
    // Event handlers
    void onProgramStart() override;
    void onProgramEnd() override;
    
private:
    void registerCommands();
    
public:
    // LED properties
    int pin = -1;
    bool active = false;
    bool ledState = false;
    int blinkInterval = 100;  // Blink interval in milliseconds
    
private:
    // Blink pattern state
    bool patternActive = false;
    unsigned long patternStartTime = 0;
    unsigned long patternLastToggle = 0;
    int patternInterval = 0;
    int patternDuration = 0;
    bool patternLedState = false;
};

#endif  // INTERNALLEDDEVICE_H