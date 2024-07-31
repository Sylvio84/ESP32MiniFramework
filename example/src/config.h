#include <Configuration.h>

#ifndef MYCONFIG_H
#define MYCONFIG_H

class MyConfig : public Configuration
{
private:

public:

    int DHT_PIN = 23;

    MyConfig() : Configuration()
    {
        HOSTNAME = "TestFrameworkESP32";
        LCD_ADDRESS = 0x3F;
        LCD_COLS = 16;
        LCD_ROWS = 2;

        DHT_PIN = 23;
    }
};

#endif
