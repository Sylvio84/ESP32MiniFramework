#ifndef DISABLE_DISPLAY
#include "Devices/DisplayLcdDevice.h"
#include <Managers/MQTTManager.h>

void DisplayLcdDevice::init()
{
    debug("DisplayLcdDevice init...", 1);
    debug("Config: " + String(cols) + "x" + String(rows) + 
          " @ 0x" + String(lcdAddress, HEX) + 
          " (SDA:" + String(sdaPin) + ", SCL:" + String(sclPin) + ")", 1);
    
    Wire.begin(sdaPin, sclPin);
    delay(100);
    
    if (!this->i2CAddrTest(lcdAddress))
    {
        debug("LCD not found at address 0x" + String(lcdAddress, HEX), 0);
        return;
    }
    
    lcd = new LiquidCrystal_I2C(lcdAddress, cols, rows);
    
    if (lcd == nullptr) {
        debug("Failed to create LCD object", 0);
        return;
    }
    
    lcd->init();
    lcd->backlight();
    lcd->clear();
    lcd->setCursor(0, 0);
    createChars();
    
    isInitialized = true;
    debug("LCD initialized successfully", 1);
    
    displayInfo();
    
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
    
    registerDeviceCommand("info", "Display LCD info", 
        [this](const std::vector<String>& params) {
            displayInfo();
            return "Info displayed on LCD";
        });
}

bool DisplayLcdDevice::i2CAddrTest(uint8_t addr)
{
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();
    return (error == 0);
}

void DisplayLcdDevice::clear()
{
    if (!isInitialized || !lcd) return;
    lcd->clear();
}

void DisplayLcdDevice::printText(uint8_t col, uint8_t row, const char *text)
{
    if (!isInitialized || !lcd) return;
    
    if (col >= cols || row >= rows) {
        debug("Invalid position: col=" + String(col) + ", row=" + String(row), 0);
        return;
    }
    
    lcd->setCursor(col, row);
    lcd->print(text);
}

void DisplayLcdDevice::printLine(uint8_t row, const char *text)
{
    if (!isInitialized || !lcd) return;
    
    if (row >= rows) {
        debug("Invalid row: " + String(row), 0);
        return;
    }
    
    lcd->setCursor(0, row);
    lcd->print(text);
    
    uint8_t textLen = strlen(text);
    for (uint8_t i = textLen; i < cols; i++) {
        lcd->print(' ');
    }
}

void DisplayLcdDevice::printLine(uint8_t row, String& text)
{
    printLine(row, text.c_str());
}

void DisplayLcdDevice::createChars()
{
    if (!lcd) return;
    
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

void DisplayLcdDevice::displayInfo()
{
    if (!isInitialized || !lcd) return;
    
    clear();
    
    String typeStr = String(cols) + "x" + String(rows);
    printLine(0, typeStr.c_str());
    
    String addrStr = "Addr: 0x" + String(lcdAddress, HEX);
    printLine(1, addrStr.c_str());
    
    if (rows >= 4) {
        String pinsStr = "SDA:" + String(sdaPin) + " SCL:" + String(sclPin);
        printLine(2, pinsStr.c_str());
        printLine(3, "Ready");
    }
    
    delay(2000);
    clear();
}

bool DisplayLcdDevice::processCommand(String command, std::vector<String> params)
{
    if (command == "clear") {
        clear();
        debug("Display cleared", 1);
        return true;
    } else if (command == "print" && params.size() >= 3) {
        printText(params[0].toInt(), params[1].toInt(), params[2].c_str());
        debug("Text printed", 1);
        return true;
    } else if (command == "line" && params.size() >= 2) {
        printLine(params[0].toInt(), params[1].c_str());
        debug("Line printed", 1);
        return true;
    } else if (command == "info") {
        displayInfo();
        debug("Info displayed", 1);
        return true;
    }
    return false;
}

bool DisplayLcdDevice::subscribeMQTT(String topic)
{
    if (topic.isEmpty()) return false;
    auto mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
    if (mqttMgr) {
        mqttMgr->subscribe(topic);
        return true;
    }
    return false;
}

bool DisplayLcdDevice::unsubscribeMQTT(String topic)
{
    if (topic.isEmpty()) return false;
    auto mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
    if (mqttMgr) {
        mqttMgr->unsubscribe(topic);
        return true;
    }
    return false;
}

bool DisplayLcdDevice::processMQTTDevice(String topic, String value)
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
    else if (topic.endsWith("/line2") && rows > 2) {
        printLine(2, value.c_str());
        return true;
    }
    else if (topic.endsWith("/line3") && rows > 3) {
        printLine(3, value.c_str());
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
    else if (topic.endsWith("/info")) {
        displayInfo();
        return true;
    }
    return false;
}

void DisplayLcdDevice::displaySystemMessage(uint8_t messageType) {
    Serial.println("[DisplayLcdDevice] displaySystemMessage called with type: " + String(messageType));
    
    if (!isInitialized) {
        Serial.println("[DisplayLcdDevice] Display not initialized, skipping message");
        return;
    }

    switch (messageType) {
        case INIT_OK:
            Serial.println("[DisplayLcdDevice] Displaying: ESP32 Ready");
            printLine(0, "ESP32 Ready");
            break;
        case DEVICE_OK:
            Serial.println("[DisplayLcdDevice] Displaying: Device OK");
            printLine(0, "Device OK");
            break;
        case WIFI_OK:
            Serial.println("[DisplayLcdDevice] Displaying: WiFi OK");
            printLine(0, "WiFi OK");
            break;
        case MQTT_OK:
            Serial.println("[DisplayLcdDevice] Displaying: MQTT OK");
            printLine(0, "MQTT OK");
            break;
        default:
            Serial.println("[DisplayLcdDevice] Unknown message type: " + String(messageType));
            printLine(0, "Unknown message");
            break;
    }
}


#endif