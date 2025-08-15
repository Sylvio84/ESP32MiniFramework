#ifndef DISABLE_DISPLAY
#include "Devices/DisplayDevice.h"
#include <Managers/MQTTManager.h>

void DisplayDevice::init()
{
    debug("DisplayDevice init...", 1);
    debug("Config: " + String(config.cols) + "x" + String(config.rows) + 
          " @ 0x" + String(config.address, HEX) + 
          " (SDA:" + String(config.sdaPin) + ", SCL:" + String(config.sclPin) + ")", 1);
    
    // Initialize I2C with configured pins
    Wire.begin(config.sdaPin, config.sclPin);
    delay(100); // Give I2C time to stabilize
    
    // Try to detect LCD if address is default or test the configured address
    bool lcdFound = false;
    if (config.address == LCD_I2C_ADDRESS) {
        // Auto-detect mode
        lcdFound = detectLCD();
    } else {
        // Test specific address
        lcdFound = testI2CAddress(config.address);
    }
    
    if (!lcdFound) {
        debug("LCD not found at address 0x" + String(config.address, HEX), 0);
        return;
    }
    
    // Create LCD object with detected/configured parameters
    lcd = new LiquidCrystal_I2C(config.address, config.cols, config.rows);
    
    if (lcd == nullptr) {
        debug("Failed to create LCD object", 0);
        return;
    }
    
    // Initialize LCD
    lcd->init();
    lcd->backlight();
    lcd->clear();
    lcd->setCursor(0, 0);
    
    isInitialized = true;
    debug("LCD initialized successfully", 1);
    
    // Display initialization info
    displayInfo();
    
    Device::init();
    
    // Register device commands
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
    
    registerDeviceCommand("info", "Display LCD info", 
        [this](const std::vector<String>& params) {
            displayInfo();
            return "Info displayed on LCD";
        });
}

bool DisplayDevice::detectLCD()
{
    debug("Auto-detecting LCD...", 1);
    
    // Common I2C addresses for LCD displays
    const uint8_t addresses[] = {0x27, 0x3F, 0x20, 0x38};
    
    for (uint8_t i = 0; i < NUM_LCD_ADDRESSES; i++) {
        uint8_t addr = addresses[i];
        if (testI2CAddress(addr)) {
            config.address = addr;
            debug("LCD detected at address 0x" + String(addr, HEX), 1);
            
            // Try to detect LCD size (this is heuristic, may need adjustment)
            // For now, we'll use the configured size
            debug("Using configured size: " + String(config.cols) + "x" + String(config.rows), 1);
            
            return true;
        }
    }
    
    debug("No LCD detected on I2C bus", 0);
    return false;
}

bool DisplayDevice::testI2CAddress(uint8_t addr)
{
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();
    return (error == 0);
}

void DisplayDevice::displayInfo()
{
    if (!isInitialized || lcd == nullptr) return;
    
    clear();
    
    // Display LCD type
    String typeStr = String(config.cols) + "x" + String(config.rows);
    printLine(0, typeStr.c_str());
    
    // Display I2C address
    String addrStr = "Addr: 0x" + String(config.address, HEX);
    printLine(1, addrStr.c_str());
    
    // For 4-line displays, show more info
    if (config.rows >= 4) {
        String pinsStr = "SDA:" + String(config.sdaPin) + " SCL:" + String(config.sclPin);
        printLine(2, pinsStr.c_str());
        printLine(3, "Ready");
    }
    
    delay(2000); // Show info for 2 seconds
    clear();
}

void DisplayDevice::clear()
{
    if (!isInitialized || lcd == nullptr) return;
    lcd->clear();
}

void DisplayDevice::printText(uint8_t col, uint8_t row, const char *text)
{
    if (!isInitialized || lcd == nullptr) return;
    
    // Bounds checking
    if (col >= config.cols || row >= config.rows) {
        debug("Invalid position: col=" + String(col) + ", row=" + String(row), 0);
        return;
    }
    
    lcd->setCursor(col, row);
    lcd->print(text);
}

void DisplayDevice::printLine(uint8_t row, const char *text)
{
    if (!isInitialized || lcd == nullptr) return;
    
    // Bounds checking
    if (row >= config.rows) {
        debug("Invalid row: " + String(row), 0);
        return;
    }
    
    lcd->setCursor(0, row);
    lcd->print(text);
    
    // Clear rest of line
    uint8_t textLen = strlen(text);
    for (uint8_t i = textLen; i < config.cols; i++) {
        lcd->print(' ');
    }
}

void DisplayDevice::printLine(uint8_t row, String& text)
{
    printLine(row, text.c_str());
}

void DisplayDevice::createChars()
{
    if (!isInitialized || lcd == nullptr) return;
    
    uint8_t bell[8] = {0x4, 0xe, 0xe, 0xe, 0x1f, 0x0, 0x4};
    uint8_t note[8] = {0x2, 0x3, 0x2, 0xe, 0x1e, 0xc, 0x0};
    uint8_t clock[8] = {0x0, 0xe, 0x15, 0x17, 0x11, 0xe, 0x0};
    uint8_t heart[8] = {0x0, 0xa, 0x1f, 0x1f, 0xe, 0x4, 0x0};
    uint8_t duck[8] = {0x0, 0xc, 0x1d, 0xf, 0xf, 0x6, 0x0};
    uint8_t check[8] = {0x0, 0x1, 0x3, 0x16, 0x1c, 0x8, 0x0};
    uint8_t cross[8] = {0x0, 0x1b, 0xe, 0x4, 0xe, 0x1b, 0x0};
    uint8_t retarrow[8] = {0x1, 0x1, 0x5, 0x9, 0x1f, 0x8, 0x4};
    
    lcd->createChar(0, bell);
    lcd->createChar(1, note);
    lcd->createChar(2, clock);
    lcd->createChar(3, heart);
    lcd->createChar(4, duck);
    lcd->createChar(5, check);
    lcd->createChar(6, cross);
    lcd->createChar(7, retarrow);
}

bool DisplayDevice::processCommand(String command, std::vector<String> params)
{
    // Process device-specific commands
    if (command == "clear") {
        clear();
        debug("Command result: Display cleared", 1);
        return true;
    } else if (command == "print" && params.size() >= 3) {
        printText(params[0].toInt(), params[1].toInt(), params[2].c_str());
        debug("Command result: Text printed", 1);
        return true;
    } else if (command == "line" && params.size() >= 2) {
        printLine(params[0].toInt(), params[1].c_str());
        debug("Command result: Line printed", 1);
        return true;
    } else if (command == "chars") {
        createChars();
        debug("Command result: Custom characters created", 1);
        return true;
    } else if (command == "info") {
        displayInfo();
        debug("Command result: Info displayed", 1);
        return true;
    }
    return false;
}

bool DisplayDevice::subscribeMQTT(String topic)
{
    if (topic.isEmpty()) return false;
    auto mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
    if (mqttMgr) {
        mqttMgr->subscribe(topic);
        return true;
    }
    return false;
}

bool DisplayDevice::unsubscribeMQTT(String topic)
{
    if (topic.isEmpty()) return false;
    auto mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
    if (mqttMgr) {
        mqttMgr->unsubscribe(topic);
        return true;
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
    else if (topic.endsWith("/line2") && config.rows > 2) {
        printLine(2, value.c_str());
        return true;
    }
    else if (topic.endsWith("/line3") && config.rows > 3) {
        printLine(3, value.c_str());
        return true;
    }
    else if (topic.endsWith("/text")) {
        // Format: "col row text"
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
    else if (topic.endsWith("/info")) {
        displayInfo();
        return true;
    }
    return false;
}

#endif