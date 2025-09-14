#include "Devices/DisplayLcdDevice.h"
#include <Managers/MQTTManager.h>

void DisplayLcdDevice::init()
{
    Device::init();

    debug("DisplayLcdDevice init...", 1);

    // Load pin configuration with intelligent defaults
    int defaultSda = getDefaultI2CPin("sda");
    int defaultScl = getDefaultI2CPin("scl");

    // Use configured defaults if auto-detection failed
    if (defaultSda == -1)
        defaultSda = sdaPin;
    if (defaultScl == -1)
        defaultScl = sclPin;

    // Load from configuration or use defaults
    sdaPin = loadPin("sda", defaultSda);
    sclPin = loadPin("scl", defaultScl);
    lcdAddress = loadPin("address", lcdAddress);  // Also allow address configuration

    // Register pins for listing
    registerPin(sdaPin, "sda", "I2C Data");
    registerPin(sclPin, "scl", "I2C Clock");

    debug("Config: " + String(cols) + "x" + String(rows) + " @ 0x" + String(lcdAddress, HEX) + " (SDA:" + String(sdaPin) + ", SCL:" + String(sclPin) + ")", 1);

    Wire.begin(sdaPin, sclPin);
    delay(500);

    if (!this->i2CAddrTest(lcdAddress)) {
        debug("LCD not found at address 0x" + String(lcdAddress, HEX), 0);
        return;
    }

    lcd = new LiquidCrystal_I2C(lcdAddress, cols, rows);

    if (lcd == nullptr) {
        debug("Failed to create LCD object", 0);
        return;
    }

    lcd->init();
    delay(100);
    lcd->backlight();
    delay(50);
    lcd->clear();
    delay(50);
    lcd->setCursor(0, 0);
    delay(50);

    printLine(0, "");
    printLine(1, "");
    printLine(2, "");
    printLine(3, "");
    delay(100);

    createChars();

    isInitialized = true;
    debug("LCD initialized successfully", 1);

    //displayInfo();

    registerDeviceCommand("clear", "Clear the display", [this](const std::vector<String>& params) {
        clear();
        return "Display cleared";
    });

    registerDeviceCommand("print", "Print text at position (col row text)", [this](const std::vector<String>& params) {
        if (params.size() < 3)
            return "Usage: print <col> <row> <text>";
        printText(params[0].toInt(), params[1].toInt(), params[2].c_str());
        return "Text printed";
    });

    registerDeviceCommand("line", "Print line (row text)", [this](const std::vector<String>& params) {
        if (params.size() < 2)
            return "Usage: line <row> <text>";
        printLine(params[0].toInt(), params[1].c_str());
        return "Line printed";
    });

    registerDeviceCommand("info", "Display LCD info", [this](const std::vector<String>& params) {
        displayInitInfo();
        return "Info displayed on LCD";
    });

    registerDeviceCommand("setsda", "Set SDA pin", [this](const std::vector<String>& params) -> String {
        if (params.size() < 1)
            return "Usage: setsda <pin>";
        int newPin = params[0].toInt();
        if (newPin < 0 || newPin > 40)
            return "ERROR: Invalid pin number";
        savePin("sda", newPin);
        return String("SDA pin set to " + String(newPin) + " (restart required)");
    });

    registerDeviceCommand("setscl", "Set SCL pin", [this](const std::vector<String>& params) -> String {
        if (params.size() < 1)
            return "Usage: setscl <pin>";
        int newPin = params[0].toInt();
        if (newPin < 0 || newPin > 40)
            return "ERROR: Invalid pin number";
        savePin("scl", newPin);
        return String("SCL pin set to " + String(newPin) + " (restart required)");
    });

    registerDeviceCommand("setaddress", "Set I2C address", [this](const std::vector<String>& params) -> String {
        if (params.size() < 1)
            return "Usage: setaddress <address>";
        int newAddr = strtol(params[0].c_str(), NULL, 0);  // Support 0x27 format
        if (newAddr < 0x08 || newAddr > 0x77)
            return "ERROR: Invalid I2C address";
        savePin("address", newAddr);
        return String("I2C address set to 0x" + String(newAddr, HEX) + " (restart required)");
    });

    timeManager->setInterval([this]() { displayEspInfo(); }, 1000);

    if (wifiManager->isConnected()) {
        displaySystemMessage(DisplayDevice::WIFI_OK);
    }
}

bool DisplayLcdDevice::i2CAddrTest(uint8_t addr)
{
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();
    return (error == 0);
}

void DisplayLcdDevice::clear()
{
    if (!isInitialized || !lcd)
        return;
    lcd->clear();
}

void DisplayLcdDevice::printText(uint8_t col, uint8_t row, const char* text)
{
    if (!isInitialized || !lcd)
        return;

    if (col >= cols) {
        debug("Invalid col: " + String(col) + "/" + String(cols), 0);
        return;
    }
    if (row >= rows) {
        debug("Invalid row: " + String(row) + "/" + String(rows), 0);
        return;
    }

    lcd->setCursor(col, row);
    lcd->print(text);
}

void DisplayLcdDevice::printLine(uint8_t row, const char* text, int col)
{
    if (!isInitialized || !lcd)
        return;

    if (row >= rows) {
        debug("Invalid row: " + String(row), 0);
        return;
    }

    lcd->setCursor(col, row);
    lcd->print(text);

    uint8_t textLen = strlen(text);
    for (uint8_t i = textLen; i < cols; i++) {
        lcd->print(' ');
    }
}

void DisplayLcdDevice::printLine(uint8_t row, String& text, int col)
{
    printLine(row, text.c_str(), col);
}

void DisplayLcdDevice::createChars()
{
    if (!lcd)
        return;

    uint8_t bell[8] = {0x4, 0xe, 0xe, 0xe, 0x1f, 0x0, 0x4};
    uint8_t note[8] = {0x2, 0x3, 0x2, 0xe, 0x1e, 0xc, 0x0};
    uint8_t clock[8] = {0x0, 0xe, 0x15, 0x17, 0x11, 0xe, 0x0};
    uint8_t mqtt[8] = {0x04, 0x0E, 0x15, 0x04, 0x04, 0x15, 0x0E, 0x04};
    uint8_t wifi[8] = {0x00, 0x00, 0x0E, 0x11, 0x04, 0x0A, 0x00, 0x04};
    uint8_t check[8] = {0x0, 0x1, 0x3, 0x16, 0x1c, 0x8, 0x0};
    uint8_t cross[8] = {0x0, 0x1b, 0xe, 0x4, 0xe, 0x1b, 0x0};
    uint8_t retarrow[8] = {0x1, 0x1, 0x5, 0x9, 0x1f, 0x8, 0x4};

    lcd->createChar(0, bell);
    lcd->createChar(1, note);
    lcd->createChar(2, clock);
    lcd->createChar(3, mqtt);
    lcd->createChar(4, wifi);
    lcd->createChar(5, check);
    lcd->createChar(6, cross);
    lcd->createChar(7, retarrow);
}

void DisplayLcdDevice::clearLine(uint8_t row)
{
    if (!isInitialized || !lcd)
        return;

    if (row >= rows) {
        debug("Invalid row: " + String(row), 0);
        return;
    }

    lcd->setCursor(0, row);
    for (uint8_t i = 0; i < cols; i++) {
        lcd->print(' ');
    }
    lcd->setCursor(0, row);
}

void DisplayLcdDevice::displayInitInfo()
{
    if (!isInitialized || !lcd)
        return;

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
        displayInitInfo();
        debug("Info displayed", 1);
        return true;
    }
    return false;
}

bool DisplayLcdDevice::subscribeMQTT(String topic)
{
    if (topic.isEmpty())
        return false;
    auto mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
    if (mqttMgr) {
        mqttMgr->subscribe(topic);
        return true;
    }
    return false;
}

bool DisplayLcdDevice::unsubscribeMQTT(String topic)
{
    if (topic.isEmpty())
        return false;
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
    } else if (topic.endsWith("/line0")) {
        printLine(0, value.c_str());
        return true;
    } else if (topic.endsWith("/line1")) {
        printLine(1, value.c_str());
        return true;
    } else if (topic.endsWith("/line2") && rows > 2) {
        printLine(2, value.c_str());
        return true;
    } else if (topic.endsWith("/line3") && rows > 3) {
        printLine(3, value.c_str());
        return true;
    } else if (topic.endsWith("/text")) {
        int firstSpace = value.indexOf(' ');
        int secondSpace = value.indexOf(' ', firstSpace + 1);
        if (firstSpace > 0 && secondSpace > firstSpace) {
            uint8_t col = value.substring(0, firstSpace).toInt();
            uint8_t row = value.substring(firstSpace + 1, secondSpace).toInt();
            String text = value.substring(secondSpace + 1);
            printText(col, row, text.c_str());
        }
        return true;
    } else if (topic.endsWith("/info")) {
        displayInitInfo();
        return true;
    }
    return false;
}

void DisplayLcdDevice::displaySystemMessage(uint8_t messageType)
{
    Serial.println("[DisplayLcdDevice] displaySystemMessage called with type: " + String(messageType));

    if (!isInitialized) {
        Serial.println("[DisplayLcdDevice] Display not initialized, skipping message");
        return;
    }

    switch (messageType) {
        case INIT_OK:
            Serial.println("[DisplayLcdDevice] Displaying: ESP32 Ready");
            printText(0, 0, "INIT");
            //auto timeManager = static_cast<TimeManager*>(context->getManager("TimeManager"));
            //TimeManager* timeManager = context ? static_cast<TimeManager*>(context->getManager("TimeManager")) : nullptr;
            break;
        case DEVICE_OK:
            Serial.println("[DisplayLcdDevice] Displaying: Device OK");
            printText(0, 0, "DISP");
            break;
        case WIFI_OK:
            Serial.println("[DisplayLcdDevice] Displaying: WiFi OK");
            lcd->setCursor(11, 0);
            lcd->write(4);  // WiFi character
            /*
            if (timeManager) {
                String currentTime = timeManager->getFormattedDateTime("%H:%M:%S");
                Serial.println("[DisplayLcdDevice] Current time: " + currentTime);
                printText(12, 0, currentTime.c_str());
                TimeManager* tm = timeManager;  // Local copy for lambda capture
                tm->setInterval(
                    [this, tm]() {
                        if (isInitialized && lcd) {
                            String currentTime = tm->getFormattedDateTime("%H:%M:%S");
                            printText(12, 0, currentTime.c_str());
                        }
                    },
                    1000);  // Update every second
            } else {
                printText(12, 0, "No TimeM");
            }
            */
            break;
        case MQTT_OK:
            Serial.println("[DisplayLcdDevice] Displaying: MQTT OK");
            lcd->setCursor(10, 0);
            lcd->write(3);  // ↕ character
            break;
        default:
            //eventManager->debug("DisplayLcdDevice: Unknown message type " + String(messageType), 0);
            break;
    }
}

void DisplayLcdDevice::displaySpecialChar(uint8_t charNum, uint8_t row, uint8_t col)
{
    if (!isInitialized || !lcd)
        return;

    if (col >= cols) {
        debug("Invalid col: " + String(col) + "/" + String(cols), 0);
        return;
    }
    if (row >= rows) {
        debug("Invalid row: " + String(row) + "/" + String(rows), 0);
        return;
    }
    if (charNum > 7) {
        debug("Invalid charNum: " + String(charNum) + " (must be 0-7)", 0);
        return;
    }

    lcd->setCursor(col, row);
    if (charNum == -1) {
        lcd->print(" ");
    } else {
        lcd->write(charNum);
    }
}

void DisplayLcdDevice::displayEspInfo()
{
    if (!isInitialized || !lcd)
        return;

    if (wifiManager->isConnected()) {
        displaySpecialChar(4, 0, 11);
    } else {
        displaySpecialChar(-1, 0, 11);
        printText(0, 1, "WiFi Disconnected");
    }

    if (mqttManager->isConnected()) {
        displaySpecialChar(3, 0, 10);
    } else {
        displaySpecialChar(-1, 0, 10);
        printText(0, 1, "MQTT Disconnected");
    }

    String currentTime = timeManager->getFormattedDateTime("%H:%M:%S");
    printText(12, 0, currentTime.c_str());
}