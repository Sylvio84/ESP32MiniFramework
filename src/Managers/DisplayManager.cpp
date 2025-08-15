#ifndef DISABLE_DISPLAY
#include "Managers/DisplayManager.h"

void DisplayManager::init()
{
    logDebug("DisplayManager init...", 1);
    logDebug("Config: " + String(cols) + "x" + String(rows) + 
          " @ 0x" + String(lcdAddress, HEX) + 
          " (SDA:" + String(sdaPin) + ", SCL:" + String(sclPin) + ")", 1);
    
    Wire.begin(sdaPin, sclPin);
    delay(100);
    
    if (!this->i2CAddrTest(lcdAddress))
    {
        logDebug("LCD not found at address 0x" + String(lcdAddress, HEX), 0);
        return;
    }
    
    lcd = new LiquidCrystal_I2C(lcdAddress, cols, rows);
    
    if (lcd == nullptr) {
        logDebug("Failed to create LCD object", 0);
        return;
    }
    
    lcd->init();
    lcd->backlight();
    lcd->clear();
    lcd->setCursor(0, 0);
    createChars();
    
    isInitialized = true;
    setInitialized(true);
    logDebug("LCD initialized successfully", 1);
}

bool DisplayManager::i2CAddrTest(uint8_t addr)
{
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();
    return (error == 0);
}

void DisplayManager::clear()
{
    if (!isInitialized || !lcd) return;
    lcd->clear();
}

void DisplayManager::printText(uint8_t col, uint8_t row, const char *text)
{
    if (!isInitialized || !lcd) return;
    
    if (col >= cols || row >= rows) {
        logDebug("Invalid position: col=" + String(col) + ", row=" + String(row), 0);
        return;
    }
    
    lcd->setCursor(col, row);
    lcd->print(text);
}

void DisplayManager::printLine(uint8_t row, const char *text)
{
    if (!isInitialized || !lcd) return;
    
    if (row >= rows) {
        logDebug("Invalid row: " + String(row), 0);
        return;
    }
    
    lcd->setCursor(0, row);
    lcd->print(text);
    
    uint8_t textLen = strlen(text);
    for (uint8_t i = textLen; i < cols; i++) {
        lcd->print(' ');
    }
}

void DisplayManager::printLine(uint8_t row, String& text)
{
    printLine(row, text.c_str());
}

void DisplayManager::createChars()
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

#endif