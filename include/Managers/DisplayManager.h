#ifndef DISABLE_DISPLAY
#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include "Manager.h"
#include <FrameworkContext.h>
#include <LiquidCrystal_I2C.h>

/**
 * Concrete DisplayManager for backward compatibility with MainController
 * For new code, use DisplayLcdDevice hierarchy instead
 */
class DisplayManager : public Manager
{
private:
    LiquidCrystal_I2C* lcd;
    uint8_t cols;
    uint8_t rows;
    uint8_t sdaPin;
    uint8_t sclPin;
    uint8_t lcdAddress;
    bool isInitialized = false;
    FrameworkContext* context;
    
public:
    DisplayManager(FrameworkContext& ctx) 
        : Manager(ctx), 
          context(&ctx), 
          lcd(nullptr),
          cols(20),    // Default to 4x20 LCD
          rows(4),      // Default to 4x20 LCD
          sdaPin(21), 
          sclPin(22), 
          lcdAddress(0x27) {}
    
    virtual ~DisplayManager() {
        if (lcd) {
            delete lcd;
        }
    }
    
    void init() override;
    
    bool i2CAddrTest(uint8_t addr);
    
    void clear();
    void printText(uint8_t col, uint8_t row, const char *text);
    void printLine(uint8_t row, const char *text);
    void printLine(uint8_t row, String &text);
    void createChars();
    
    String getName() const override { return "DisplayManager"; }
    
    LiquidCrystal_I2C* getLcd() { return lcd; }
};

#endif
#endif