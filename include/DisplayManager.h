#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Configuration.h>
#include <LiquidCrystal_I2C.h> // lib_deps = marcoschwartz/LiquidCrystal_I2C@^1.1.4

class DisplayManager
{
private:
    LiquidCrystal_I2C lcd;
    uint8_t cols;
    uint8_t rows;
    uint8_t sdaPin;
    uint8_t sclPin;
    uint8_t lcdAddress;

public:
    DisplayManager(Configuration config);

    bool init();

    bool i2CAddrTest(uint8_t addr);

    void clear();

    // Print text at a specific position
    void printText(uint8_t col, uint8_t row, const char *text);

    // Print a single line, overwriting it
    void printLine(uint8_t row, const char *text);
    void printLine(uint8_t row, String &text);

    void createChars();

    LiquidCrystal_I2C getLcd();
};

#endif