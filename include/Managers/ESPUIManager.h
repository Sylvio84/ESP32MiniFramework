#ifndef DISABLE_ESPUI
#ifndef ESPUIMANAGER_H
#define ESPUIMANAGER_H

#include <ESPUI.h>
#include <Arduino.h>
#include "Manager.h"
#include <FrameworkContext.h>
#include <vector>

// Forward declarations
class CommandManager;

class ESPUIManager : public Manager
{

private:
    FrameworkContext* context;

    // callback
    void (*callback)(Control *sender, int type);


    uint16_t commandText = 0;
    uint16_t sendCommandButton = 0;
    uint16_t debugLabel = 0;

    std::vector<String> debugMessages;

public:
    // New constructor with FrameworkContext
    ESPUIManager(FrameworkContext& ctx) : Manager(ctx), context(&ctx) { }
    

    void init() override;
    bool onCommand(const String& command, const std::vector<String>& params) override;
    void onEvent(const String& type, const String& event, const std::vector<String>& params) override;

    void print(uint8_t labelId, String text);
    void addDebugMessage(String message, int level = 0);

    uint16_t initInfoTab();
    uint16_t initDebugTab();
    
    void EspUiCallback(Control *sender, int type);
    
    String getName() const override { return "ESPUIManager"; }
    
    void registerCommands();

    Control* getControl(uint16_t id);
};

#endif
#endif