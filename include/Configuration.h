#ifndef CONFIGURATION_H
#define CONFIGURATION_H

class Configuration
{
public:
    Configuration() {}

    int CONNECTION_TIMEOUT = 10000;

    const char *HOSTNAME = "ESP32";

    int LCD_ADDRESS = 0x27;
    int LCD_COLS = 20;
    int LCD_ROWS = 4;
    int LCD_SDA = 13;
    int LCD_SCL = 14;

    const char *TIMEZONE = "CET-1CEST,M3.5.0,M10.5.0/3";
    const char *NTP_SERVER = "pool.ntp.org";
    const char *DATE_FORMAT = "%d/%m/%Y";
    const char *TIME_FORMAT = "%H:%M:%S";
    const char *DATETIME_FORMAT = "%d/%m/%Y %H:%M:%S";

    static int getValue(const char *key, int defaultValue = 0)
    {
        return defaultValue;
    }

    static String getValue(const char *key, String defaultValue = "")
    {
        return defaultValue;
    }
};

#endif
