#ifndef ESPUIMANAGER_H
#define ESPUIMANAGER_H

#include <ESPUI.h>
#include <Arduino.h>
#include <Configuration.h>
#include <EventManager.h>
#include <vector>

class ESPUIManager
{

private:

    Configuration& config;
    // callback
    void (*callback)(Control *sender, int type);

    static EventManager *eventManager; // Pointeur vers EventManager

    uint16_t commandText = 0;
    uint16_t sendCommandButton = 0;
    uint16_t debugLabel = 0;

    std::vector<String> debugMessages;

public:
    ESPUIManager(Configuration& config, EventManager &eventMgr): config(config)
    {
        if (eventManager == nullptr)
        {
            eventManager = &eventMgr;
        }
    }

    void init();
    void processCommand(String command);
    void processEvent(String type, String event, std::vector<String> params);

    void print(uint8_t labelId, String text);
    void addDebugMessage(String message, int level = 0);

    uint16_t initInfoTab();
    uint16_t initDebugTab();
    
    void EspUiCallback(Control *sender, int type);
    
    Control* getControl(uint16_t id);
};

#endif
