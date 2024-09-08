#ifndef SERIAL_COMMAND_MANAGER_H
#define SERIAL_COMMAND_MANAGER_H

#include <Arduino.h>
#include <Configuration.h>
#include <EventManager.h>
#include <functional>
#include <map>
#include <vector>

class SerialCommandManager
{
  public:
    SerialCommandManager(Configuration& config, EventManager& eventMgr) : config(config)
    {
        baudRate = config.getPreference("serial_speed", baudRate);
        if (eventManager == nullptr) {
            eventManager = &eventMgr;
        }
    }

    void init();
    void loop();

    void processCommand(const String& input);

  private:
    int baudRate = 115200;
    Configuration& config;

    void handleSerialInput();
    std::vector<String> splitParameters(const String& paramStr);

    static EventManager* eventManager;
};

#endif  // SERIAL_COMMAND_MANAGER_H
