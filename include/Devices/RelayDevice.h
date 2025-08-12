#ifndef RELAYDEVICE_H
#define RELAYDEVICE_H

#include <Arduino.h>
#include <Devices/OnOffDevice.h>

class RelayDevice : public OnOffDevice
{
public:
    // Constructor
    RelayDevice(String id, FrameworkContext& ctx);
    
    // Virtual method implementations
    void init() override;

protected:
    // Register relay-specific commands
    void registerSpecificCommands() override;
};

#endif  // RELAYDEVICE_H