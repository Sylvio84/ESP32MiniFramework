#ifndef DISABLE_DISPLAY
#ifndef DISPLAYDEVICE_H
#define DISPLAYDEVICE_H

#include <Arduino.h>
#include <Wire.h>
#include "Device.h"
#include <FrameworkContext.h>
#include <LiquidCrystal_I2C.h>

class DisplayDevice : public Device
{
private:
    LiquidCrystal_I2C lcd;
    uint8_t cols;
    uint8_t rows;
    uint8_t sdaPin;
    uint8_t sclPin;
    uint8_t lcdAddress;
    bool isInitialized = false;
    
public:
    DisplayDevice(String id, FrameworkContext& ctx) : 
        Device(id, ctx), 
        lcd(0x27, 16, 2), 
        cols(16), 
        rows(2), 
        sdaPin(21), 
        sclPin(22), 
        lcdAddress(0x27) 
    {
        type = "display";
        name = "Display";
    }
    
    void init() override;
    void loop() override {}
    
    bool processCommand(String command, std::vector<String> params) override;
    bool processUI(String action, std::vector<String> params) override { return false; }
    void processEvent(String type, String event, std::vector<String> params) override {}
    
    bool subscribeMQTT(String topic) override;
    bool unsubscribeMQTT(String topic) override;
    
    bool i2CAddrTest(uint8_t addr);
    void clear();
    void printText(uint8_t col, uint8_t row, const char *text);
    void printLine(uint8_t row, const char *text);
    void printLine(uint8_t row, String &text);
    void createChars();
    
    LiquidCrystal_I2C getLcd() { return lcd; }
    
protected:
    bool processMQTTDevice(String topic, String value) override;
};

#endif
#endif