#ifndef DISABLE_DISPLAY
#ifndef DISPLAYLCD4X20DEVICE_H
#define DISPLAYLCD4X20DEVICE_H

#include "DisplayLcdDevice.h"

/**
 * LCD 4x20 Display Device
 * Manages a 20 columns x 4 rows LCD display
 */
class DisplayLcd4x20Device : public DisplayLcdDevice
{
public:
    DisplayLcd4x20Device(String id, FrameworkContext& ctx, uint8_t lcdAddress = 0x27, 
                         uint8_t sdaPin = 21, uint8_t sclPin = 22)
        : DisplayLcdDevice(id, ctx, 20, 4, lcdAddress, sdaPin, sclPin) 
    {
        name = "LCD 4x20 Display";
    }
};

#endif
#endif