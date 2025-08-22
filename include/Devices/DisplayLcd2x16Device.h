#ifndef DISPLAYLCD2X16DEVICE_H
#define DISPLAYLCD2X16DEVICE_H

#include "DisplayLcdDevice.h"

/**
 * LCD 2x16 Display Device
 * Manages a 16 columns x 2 rows LCD display
 */
class DisplayLcd2x16Device : public DisplayLcdDevice
{
public:
    DisplayLcd2x16Device(String id, FrameworkContext& ctx, uint8_t lcdAddress = 0x27, 
                         uint8_t sdaPin = 21, uint8_t sclPin = 22)
        : DisplayLcdDevice(id, ctx, 16, 2, lcdAddress, sdaPin, sclPin) 
    {
        name = "LCD 2x16 Display";
    }
};

#endif