#ifndef INTERNALLEDDEVICE_H
#define INTERNALLEDDEVICE_H

#include <Arduino.h>
#include <Devices/OnOffDevice.h>
#ifdef ESP32
#include <esp_chip_info.h>
#endif

class InternalLedDevice : public OnOffDevice
{
public:
    InternalLedDevice(String id, FrameworkContext& ctx);
    
    void init() override;
    void loop() override;
    
    // Override activate/deactivate to manage 'active' property
    void activate(int timeout = 0) override;
    void deactivate() override;
    
    // LED-specific methods
    void detectLedPin();
    bool getActive();
    void startBlinkPattern(int intervalMs, int durationMs);
    void processBlinkPattern();

protected:
    // Register LED-specific commands (blink)
    void registerSpecificCommands() override;
    
    // MQTT processing for LED-specific commands
    bool processMQTTDevice(String topic, String value) override;

public:
    // LED-specific properties
    bool active = false;          // Blink mode active
    int blinkInterval = 0;         // Blink interval in milliseconds (0 = no blinking)
    
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