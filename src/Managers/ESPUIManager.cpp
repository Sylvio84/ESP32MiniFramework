#ifndef DISABLE_ESPUI
#include "Managers/ESPUIManager.h"
#include "Managers/ConfigurationManager.h"
#include "Managers/EventManager.h"
#include "Managers/CommandManager.h"


void ESPUIManager::init()
{
    logDebug("ESPUIManager init...", 1);
    
    // Register ESPUI commands with CommandManager
    registerCommands();
    
    setInitialized(true);
    ESPUI.setVerbosity(Verbosity::Quiet);

    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    String hostname = configMgr ? configMgr->getHostname() : "ESP32";
    ESPUI.begin(hostname.c_str());
    initInfoTab();
    initDebugTab();
}

uint16_t ESPUIManager::initInfoTab()
{
    auto infoTab = ESPUI.addControl(Tab, "", "Info");
    return infoTab;
}

uint16_t ESPUIManager::initDebugTab()
{
    auto callback = std::bind(&ESPUIManager::EspUiCallback, this, std::placeholders::_1, std::placeholders::_2);

    auto debugTab = ESPUI.addControl(Tab, "", "Debug");
    // serialLabelId = ESPUI.addControl(Label, "Serial", "Serial IN", Peterriver, maintab, textCallback);
    // statusLabelId = ESPUI.addControl(Label, "", "Serial OUT", Peterriver, serialLabelId, textCallback);
    commandText = ESPUI.addControl(Text, "Command", "", Peterriver, debugTab, callback);
    sendCommandButton = ESPUI.addControl(Button, "Send", "SendCommand", Peterriver, commandText, callback);
    debugLabel = ESPUI.addControl(Label, "Debug Logs", "", Peterriver, debugTab, callback);

    auto reboot = ESPUI.addControl(Button, "Reboot", "Reboot", Peterriver, debugTab, callback);
    ESPUI.setEnabled(reboot, true);

    return debugTab;
}


void ESPUIManager::onEvent(const String& type, const String& event, const std::vector<String>& params)
{
    if (type == "espui")
    {
        if (event == "addDebug")
        {
            ESPUI.print(debugLabel, params[0]);
        }
    }
}

void ESPUIManager::print(uint8_t labelId, String text)
{
    ESPUI.print(labelId, text);
}

void ESPUIManager::addDebugMessage(String message, int level)
{
    debugMessages.push_back(message);
    // rotate messages
    if (debugMessages.size() > 20)
    {
        debugMessages.erase(debugMessages.begin());
    }
    String text = "";
    for (auto &msg : debugMessages)
    {
        text += msg + "\n";
    }
    ESPUI.print(debugLabel, text);
}

void ESPUIManager::EspUiCallback(Control *sender, int type)
{
    logDebug("Button callback: sender.value = " + sender->value + " sender.id = " + sender->id + " sender.type = " + sender->type + "  / type = " + String(type), 2);
    //eventManager->debug("Button callback: sender.value = " + sender->value + " sender.id = " + sender->id + " sender.type = " + sender->type + "  / type = " + String(type), 1); // => don't uncomment, else reboot while loading ESPUI interface
    if (type == B_DOWN)
    {
        return;
    }

    if (sender->value == "Reboot")
    {
        context->getEventManager()->triggerEvent("espui", "Reboot", {});
    }

    if (sender->value == "SendCommand")
    {
        context->getEventManager()->triggerEvent("espui", "Command", {ESPUI.getControl(commandText)->value});
    }
}

Control *ESPUIManager::getControl(uint16_t id)
{
    return ESPUI.getControl(id);
}

void ESPUIManager::registerCommands()
{
    auto* cmdMgr = static_cast<CommandManager*>(context ? context->getManager("CommandManager") : nullptr);
    if (!cmdMgr) return;

    // ESPUI status command
    cmdMgr->registerCommand(Command(
        "espui", "status", "Show ESPUI web interface status",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            return String("ESPUI web interface is ") + (isInitialized() ? "running" : "not running");
        }
    ));

    // ESPUI debug command (add debug message to web interface)
    cmdMgr->registerCommand(Command(
        "espui", "debug", "Add debug message to web interface",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            if (args.size() == 0) {
                return "Usage: espui:debug <message>";
            }
            
            String message = args[0];
            // Join all args if multiple words
            for (size_t i = 1; i < args.size(); i++) {
                message += " " + args[i];
            }
            
            addDebugMessage(message, 0);
            return "Debug message added to web interface";
        }
    ));

    // ESPUI restart command 
    cmdMgr->registerCommand(Command(
        "espui", "restart", "Restart ESPUI web interface",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            // Restart ESPUI by triggering reboot event
            context->getEventManager()->triggerEvent("espui", "Reboot", {});
            return "ESPUI restart triggered";
        }
    ));

}
#endif