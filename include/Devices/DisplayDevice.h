#ifndef DISABLE_DISPLAY
#ifndef DISPLAYDEVICE_H
#define DISPLAYDEVICE_H

#include <Arduino.h>
#include <Wire.h>
#include "Device.h"
#include <FrameworkContext.h>
#include <LiquidCrystal_I2C.h>

// Default LCD configurations if not defined
#ifndef LCD_I2C_ADDRESS
    #define LCD_I2C_ADDRESS 0x27
#endif

#ifndef LCD_COLS
    #ifdef LCD_TYPE_4X20
        #define LCD_COLS 20
    #else
        #define LCD_COLS 16  // Default 2x16
    #endif
#endif

#ifndef LCD_ROWS
    #ifdef LCD_TYPE_4X20
        #define LCD_ROWS 4
    #else
        #define LCD_ROWS 2   // Default 2x16
    #endif
#endif

#ifndef LCD_SDA_PIN
    #define LCD_SDA_PIN 21  // Default for ESP32
#endif

#ifndef LCD_SCL_PIN
    #define LCD_SCL_PIN 22  // Default for ESP32
#endif

class DisplayDevice : public Device
{
public:
    struct LCDConfig {
        uint8_t address;
        uint8_t cols;
        uint8_t rows;
        uint8_t sdaPin;
        uint8_t sclPin;
    };

private:
    LiquidCrystal_I2C* lcd = nullptr;  // Pointer for deferred initialization
    LCDConfig config;
    bool isInitialized = false;
    
    // Common I2C addresses for LCD displays
    static const uint8_t NUM_LCD_ADDRESSES = 4;
    
    // Auto-detect LCD on I2C bus
    bool detectLCD();
    bool testI2CAddress(uint8_t addr);
    
public:
    // Constructor with optional runtime configuration
    DisplayDevice(String id, FrameworkContext& ctx, 
                  uint8_t cols = 0, uint8_t rows = 0, 
                  uint8_t address = 0, uint8_t sdaPin = 0, uint8_t sclPin = 0) : 
        Device(id, ctx)
    {
        type = "display";
        name = "LCD Display";
        
        // Use runtime params if provided, otherwise use build-time defaults
        config.cols = (cols > 0) ? cols : LCD_COLS;
        config.rows = (rows > 0) ? rows : LCD_ROWS;
        config.address = (address > 0) ? address : LCD_I2C_ADDRESS;
        config.sdaPin = (sdaPin > 0) ? sdaPin : LCD_SDA_PIN;
        config.sclPin = (sclPin > 0) ? sclPin : LCD_SCL_PIN;
    }
    
    ~DisplayDevice() {
        if (lcd != nullptr) {
            delete lcd;
            lcd = nullptr;
        }
    }
    
    void init() override;
    void loop() override {}
    
    bool processCommand(String command, std::vector<String> params) override;
    bool processUI(String action, std::vector<String> params) override { return false; }
    void processEvent(String type, String event, std::vector<String> params) override {}
    
    bool subscribeMQTT(String topic) override;
    bool unsubscribeMQTT(String topic) override;
    
    // LCD operations
    void clear();
    void printText(uint8_t col, uint8_t row, const char *text);
    void printLine(uint8_t row, const char *text);
    void printLine(uint8_t row, String &text);
    void createChars();
    
    // Getters
    LiquidCrystal_I2C* getLcd() { return lcd; }
    const LCDConfig& getConfig() const { return config; }
    bool getIsInitialized() const { return isInitialized; }
    
    // Info display
    void displayInfo();
    
protected:
    bool processMQTTDevice(String topic, String value) override;
};

#endif
#endif