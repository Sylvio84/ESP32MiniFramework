#ifndef DISABLE_DISPLAY
#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Manager.h>
#include <FrameworkContext.h>
#include <LiquidCrystal_I2C.h> // lib_deps = marcoschwartz/LiquidCrystal_I2C@^1.1.4

/*
@Todo: to transform into a Device child class
*/

class DisplayManager : public Manager
{
private:
    LiquidCrystal_I2C lcd;
    uint8_t cols;
    uint8_t rows;
    uint8_t sdaPin;
    uint8_t sclPin;
    uint8_t lcdAddress;

    bool isInitialized = false;

private:
    FrameworkContext* context;
    
public:
    // New constructor with FrameworkContext
    DisplayManager(FrameworkContext& ctx) : Manager(ctx), context(&ctx), lcd(0x27, 16, 2), cols(16), rows(2), sdaPin(21), sclPin(22), lcdAddress(0x27) {}
    

    void init() override;

    bool i2CAddrTest(uint8_t addr);

    void clear();

    // Print text at a specific position
    void printText(uint8_t col, uint8_t row, const char *text);

    // Print a single line, overwriting it
    void printLine(uint8_t row, const char *text);
    void printLine(uint8_t row, String &text);

    void createChars();

    String getName() const override { return "DisplayManager"; }

    LiquidCrystal_I2C getLcd();
};

#endif
#endif
