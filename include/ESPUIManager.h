#ifndef ESPUIMANAGER_H
#define ESPUIMANAGER_H

#include <ESPUI.h>
#include <Arduino.h>
#include <Configuration.h>
#include <EventManager.h>

class ESPUIManager
{

private:
    const char *hostname;

    // callback
    void (*callback)(Control *sender, int type);

    static EventManager *eventManager; // Pointeur vers EventManager

    uint16_t serialLabel = 0;
    uint16_t statusLabel = 0;

public:
    ESPUIManager(Configuration& config, EventManager &eventMgr)
    {
        this->hostname = config.HOSTNAME;
        if (eventManager == nullptr)
        {
            eventManager = &eventMgr;
        }
    }

    void init();
    void processCommand(String command);
    void print(uint8_t labelId, String text);

    uint16_t initInfoTab();
    uint16_t initDebugTab();
    
    void EspUiCallback(Control *sender, int type);
    
    Control* getControl(uint16_t id);
};

#endif
