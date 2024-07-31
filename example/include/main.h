bool connectWifi();

bool initWifi(const char *ssid, const char *password, const char *name);

bool initMQTT();

bool reconnectMqtt();

bool initTime();

void initLCD();

void initOTA();

bool i2CAddrTest(uint8_t addr);

void logMessage(String message, String type = "info", bool sendOnInternet = true);

bool sendMessage(String message, String type);

void asyncLogMessage(void *pvParameters);

String getFormattedDateTime(const char *format = "%d/%m/%Y %H:%M:%S");

int managePompe();

void callbackMQTT(char *topic, byte *payload, unsigned int length);

void handlePinState(int pin, byte state);

void scrollMessage(int row, String message, int delayTime, int totalColumns);

void readProgArrosage();