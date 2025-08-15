#ifndef DISABLE_DISPLAY
#ifndef DISPLAYDEVICE_H
#define DISPLAYDEVICE_H

#include <Arduino.h>
#include "Device.h"
#include <FrameworkContext.h>

/**
 * Base class for all display devices
 * Provides common interface for different display types
 */
class DisplayDevice : public Device
{
protected:
    uint8_t cols;
    uint8_t rows;
    bool isInitialized = false;
    
public:
    // System message constants
    static constexpr uint8_t INIT_OK = 0;
    static constexpr uint8_t DEVICE_OK = 1;
    static constexpr uint8_t WIFI_OK = 2;
    static constexpr uint8_t MQTT_OK = 3;
    
    DisplayDevice(String id, FrameworkContext& ctx, uint8_t cols = 16, uint8_t rows = 2) 
        : Device(id, ctx), cols(cols), rows(rows) 
    {
        type = "display";
        name = "Display Device";
    }
    
    virtual void init() override = 0;
    virtual void loop() override {}
    
    virtual bool processCommand(String command, std::vector<String> params) override = 0;
    virtual bool processUI(String action, std::vector<String> params) override { return false; }
    virtual void processEvent(String type, String event, std::vector<String> params) override;
    
    virtual bool subscribeMQTT(String topic) override = 0;
    virtual bool unsubscribeMQTT(String topic) override = 0;
    
    virtual void clear() = 0;
    virtual void printText(uint8_t col, uint8_t row, const char *text) = 0;
    virtual void printLine(uint8_t row, const char *text) = 0;
    virtual void printLine(uint8_t row, String &text) = 0;
    
    // Display system messages
    virtual void displaySystemMessage(uint8_t messageType);
    
    uint8_t getCols() const { return cols; }
    uint8_t getRows() const { return rows; }
    bool getIsInitialized() const { return isInitialized; }
    
protected:
    virtual bool processMQTTDevice(String topic, String value) override = 0;
};

#endif
#endif