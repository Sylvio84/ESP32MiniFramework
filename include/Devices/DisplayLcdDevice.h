#ifndef DISABLE_DISPLAY
#ifndef DISPLAYLCDDEVICE_H
#define DISPLAYLCDDEVICE_H

#include <Arduino.h>
#include <Wire.h>
#include "DisplayDevice.h"
#include <LiquidCrystal_I2C.h>

/**
 * LCD Display Device base class
 * Provides common functionality for LCD displays using I2C
 */
class DisplayLcdDevice : public DisplayDevice
{
protected:
    LiquidCrystal_I2C* lcd;
    uint8_t sdaPin;
    uint8_t sclPin;
    uint8_t lcdAddress;
    
    bool i2CAddrTest(uint8_t addr);
    void createChars();
    
public:
    DisplayLcdDevice(String id, FrameworkContext& ctx, uint8_t cols = 16, uint8_t rows = 2, 
                     uint8_t lcdAddress = 0x27, uint8_t sdaPin = 21, uint8_t sclPin = 22)
        : DisplayDevice(id, ctx, cols, rows), 
          lcd(nullptr), 
          sdaPin(sdaPin), 
          sclPin(sclPin), 
          lcdAddress(lcdAddress) 
    {
        name = "LCD Display";
    }
    
    virtual ~DisplayLcdDevice() {
        if (lcd) {
            delete lcd;
        }
    }
    
    virtual void init() override;
    
    virtual bool processCommand(String command, std::vector<String> params) override;
    virtual bool subscribeMQTT(String topic) override;
    virtual bool unsubscribeMQTT(String topic) override;
    
    virtual void clear() override;
    virtual void printText(uint8_t col, uint8_t row, const char *text) override;
    virtual void printLine(uint8_t row, const char *text) override;
    virtual void printLine(uint8_t row, String &text) override;
    
    LiquidCrystal_I2C* getLcd() { return lcd; }
    
    void displayInfo();

    void displaySystemMessage(uint8_t messageType) override;

    
protected:
    virtual bool processMQTTDevice(String topic, String value) override;
};

#endif
#endif