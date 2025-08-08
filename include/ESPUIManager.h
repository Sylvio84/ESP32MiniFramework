#ifndef DISABLE_ESPUI
#ifndef ESPUIMANAGER_H
#define ESPUIMANAGER_H

#include <ESPUI.h>
#include <Arduino.h>
#include <FrameworkContext.h>
#include <vector>

class ESPUIManager
{

private:

    FrameworkContext* context;
    // callback
    void (*callback)(Control *sender, int type);

    static EventManager *eventManager; // Pointeur vers EventManager

    uint16_t commandText = 0;
    uint16_t sendCommandButton = 0;
    uint16_t debugLabel = 0;

    std::vector<String> debugMessages;

public:
    // New constructor with FrameworkContext
    ESPUIManager(FrameworkContext& ctx) : context(&ctx)
    {
        if (eventManager == nullptr)
        {
            eventManager = ctx.getService<EventManager>();
        }
    }
    
    // Legacy constructor for compatibility
    ESPUIManager(Configuration& config, EventManager &eventMgr): context(nullptr)
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
#endif