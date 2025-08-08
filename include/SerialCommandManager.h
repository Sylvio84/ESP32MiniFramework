#ifndef SERIAL_COMMAND_MANAGER_H
#define SERIAL_COMMAND_MANAGER_H

#include <Arduino.h>
#include <FrameworkContext.h>
#include <functional>
#include <map>
#include <vector>

class SerialCommandManager
{
  public:
    // New constructor with FrameworkContext
    SerialCommandManager(FrameworkContext& ctx);
    
    // Legacy constructor for compatibility
    SerialCommandManager(Configuration& config, EventManager& eventMgr);

    void init();
    void loop();

  private:
    int baudRate = 115200;
    FrameworkContext* context;

    String inputBuffer;

    void handleSerialInput();

    static EventManager* eventManager;
};

#endif  // SERIAL_COMMAND_MANAGER_H
