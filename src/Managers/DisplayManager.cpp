#ifndef DISABLE_DISPLAY
#include "Managers/DisplayManager.h"




void DisplayManager::init()
{
    logDebug("DisplayManager init...", 1);
    
    // Use default values for LCD configuration
    // In future, these could be read from ConfigurationManager if needed
    this->lcd = LiquidCrystal_I2C(0x27, 16, 2);
    this->cols = 16;
    this->rows = 2;
    this->sdaPin = 21;
    this->sclPin = 22;
    this->lcdAddress = 0x27;
    Wire.begin(sdaPin, sclPin); // Initialize I2C with specific SDA and SCL pins
    if (!this->i2CAddrTest(lcdAddress))
    {
        logDebug("LCD not found", 0);
        return;
    }
    isInitialized = true;
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    setInitialized(true);
}

bool DisplayManager::i2CAddrTest(uint8_t addr)
{
    Wire.begin();
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
    {
        return true;
    }
    return false;
}

// Clear the display
void DisplayManager::clear()
{
    if (!isInitialized)
    {
        return;
    }
    lcd.clear();
}

// Print text at a specific position
void DisplayManager::printText(uint8_t row, uint8_t col, const char *text)
{
    if (!isInitialized)
    {
        return;
    }
    lcd.setCursor(col, row);
    lcd.print(text);
}

// Print a single line, overwriting it
void DisplayManager::printLine(uint8_t row, const char *text)
{
    if (!isInitialized)
    {
        return;
    }
    lcd.setCursor(0, row);
    lcd.print(text);
    // Clear the rest of the line if text is shorter than the line length
    for (uint8_t i = strlen(text); i < this->cols; i++)
    {
        lcd.print(' ');
    }
}

void DisplayManager::printLine(uint8_t row, String& text)
{
    if (!isInitialized)
    {
        return;
    }
    lcd.setCursor(0, row);
    lcd.print(text);
    // Clear the rest of the line if text is shorter than the line length
    for (uint8_t i = text.length(); i < this->cols; i++)
    {
        lcd.print(' ');
    }
}


void DisplayManager::createChars()
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

LiquidCrystal_I2C DisplayManager::getLcd()
{
    return lcd;
}
#endif
