#ifndef DISABLE_DISPLAY
#include "Devices/DisplayDevice.h"
#include <Configuration.h>

void DisplayDevice::init()
{
    debug("DisplayDevice init...", 1);
    
    Configuration* config = context ? context->getConfiguration() : nullptr;
    if (config) {
        this->lcd = LiquidCrystal_I2C(config->LCD_ADDRESS, config->LCD_COLS, config->LCD_ROWS);
        this->cols = config->LCD_COLS;
        this->rows = config->LCD_ROWS;
        this->sdaPin = config->LCD_SDA;
        this->sclPin = config->LCD_SCL;
        this->lcdAddress = config->LCD_ADDRESS;
    } else {
        this->lcd = LiquidCrystal_I2C(0x27, 16, 2);
        this->cols = 16;
        this->rows = 2;
        this->sdaPin = 21;
        this->sclPin = 22;
        this->lcdAddress = 0x27;
    }
    
    Wire.begin(sdaPin, sclPin);
    if (!this->i2CAddrTest(lcdAddress))
    {
        debug("LCD not found", 0);
        return;
    }
    
    isInitialized = true;
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    
    Device::init();
    
    registerDeviceCommand("clear", "Clear the display", 
        [this](const std::vector<String>& params) {
            clear();
            return "Display cleared";
        });
    
    registerDeviceCommand("print", "Print text at position (col row text)", 
        [this](const std::vector<String>& params) {
            if (params.size() < 3) return "Usage: print <col> <row> <text>";
            printText(params[0].toInt(), params[1].toInt(), params[2].c_str());
            return "Text printed";
        });
    
    registerDeviceCommand("line", "Print line (row text)", 
        [this](const std::vector<String>& params) {
            if (params.size() < 2) return "Usage: line <row> <text>";
            printLine(params[0].toInt(), params[1].c_str());
            return "Line printed";
        });
    
    registerDeviceCommand("chars", "Create custom characters", 
        [this](const std::vector<String>& params) {
            createChars();
            return "Custom characters created";
        });
}

bool DisplayDevice::i2CAddrTest(uint8_t addr)
{
    Wire.begin();
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
    {
        return true;
    }
    return false;
}

void DisplayDevice::clear()
{
    if (!isInitialized)
    {
        return;
    }
    lcd.clear();
}

void DisplayDevice::printText(uint8_t row, uint8_t col, const char *text)
{
    if (!isInitialized)
    {
        return;
    }
    lcd.setCursor(col, row);
    lcd.print(text);
}

void DisplayDevice::printLine(uint8_t row, const char *text)
{
    if (!isInitialized)
    {
        return;
    }
    lcd.setCursor(0, row);
    lcd.print(text);
    for (uint8_t i = strlen(text); i < this->cols; i++)
    {
        lcd.print(' ');
    }
}

void DisplayDevice::printLine(uint8_t row, String& text)
{
    if (!isInitialized)
    {
        return;
    }
    lcd.setCursor(0, row);
    lcd.print(text);
    for (uint8_t i = text.length(); i < this->cols; i++)
    {
        lcd.print(' ');
    }
}

void DisplayDevice::createChars()
{
    uint8_t bell[8] = {0x4, 0xe, 0xe, 0xe, 0x1f, 0x0, 0x4};
    uint8_t note[8] = {0x2, 0x3, 0x2, 0xe, 0x1e, 0xc, 0x0};
    uint8_t clock[8] = {0x0, 0xe, 0x15, 0x17, 0x11, 0xe, 0x0};
    uint8_t heart[8] = {0x0, 0xa, 0x1f, 0x1f, 0xe, 0x4, 0x0};
    uint8_t duck[8] = {0x0, 0xc, 0x1d, 0xf, 0xf, 0x6, 0x0};
    uint8_t check[8] = {0x0, 0x1, 0x3, 0x16, 0x1c, 0x8, 0x0};
    uint8_t cross[8] = {0x0, 0x1b, 0xe, 0x4, 0xe, 0x1b, 0x0};
    uint8_t retarrow[8] = {0x1, 0x1, 0x5, 0x9, 0x1f, 0x8, 0x4};
    lcd.createChar(0, bell);
    lcd.createChar(1, note);
    lcd.createChar(2, clock);
    lcd.createChar(3, heart);
    lcd.createChar(4, duck);
    lcd.createChar(5, check);
    lcd.createChar(6, cross);
    lcd.createChar(7, retarrow);
}

bool DisplayDevice::processCommand(String command, std::vector<String> params)
{
    for (const auto& cmd : deviceCommands) {
        if (cmd.name == command) {
            String result = cmd.handler(params);
            debug("Command result: " + result, 1);
            return true;
        }
    }
    return false;
}

bool DisplayDevice::subscribeMQTT(String topic)
{
    if (topic.isEmpty()) return false;
    auto mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
    if (mqttMgr) {
        return mqttMgr->subscribe(topic);
    }
    return false;
}

bool DisplayDevice::unsubscribeMQTT(String topic)
{
    if (topic.isEmpty()) return false;
    auto mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
    if (mqttMgr) {
        return mqttMgr->unsubscribe(topic);
    }
    return false;
}

bool DisplayDevice::processMQTTDevice(String topic, String value)
{
    if (topic.endsWith("/clear")) {
        clear();
        return true;
    }
    else if (topic.endsWith("/line0")) {
        printLine(0, value.c_str());
        return true;
    }
    else if (topic.endsWith("/line1")) {
        printLine(1, value.c_str());
        return true;
    }
    else if (topic.endsWith("/text")) {
        int firstSpace = value.indexOf(' ');
        int secondSpace = value.indexOf(' ', firstSpace + 1);
        if (firstSpace > 0 && secondSpace > firstSpace) {
            uint8_t col = value.substring(0, firstSpace).toInt();
            uint8_t row = value.substring(firstSpace + 1, secondSpace).toInt();
            String text = value.substring(secondSpace + 1);
            printText(col, row, text.c_str());
        }
        return true;
    }
    return false;
}

#endif