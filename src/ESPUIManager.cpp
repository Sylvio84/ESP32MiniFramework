#include "../include/ESPUIManager.h"

// Définition et initialisation du membre statique
EventManager *ESPUIManager::eventManager = nullptr;

void ESPUIManager::init()
{
    Serial.println("ESPUIManager init...");
    ESPUI.setVerbosity(Verbosity::Quiet);

    ESPUI.begin(config.getHostname().c_str());
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

void ESPUIManager::processCommand(String command)
{
}

void ESPUIManager::processEvent(String type, String event, std::vector<String> params)
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
    Serial.println("Button callback: sender.value = " + sender->value + " sender.id = " + sender->id + " sender.type = " + sender->type + "  / type = " + String(type));
    //eventManager->debug("Button callback: sender.value = " + sender->value + " sender.id = " + sender->id + " sender.type = " + sender->type + "  / type = " + String(type), 1); // => don't uncomment, else reboot while loading ESPUI interface
    if (type == B_DOWN)
    {
        return;
    }

    if (sender->value == "Reboot")
    {
        eventManager->triggerEvent("espui", "Reboot", {});
    }

    if (sender->value == "SendCommand")
    {
        eventManager->triggerEvent("espui", "Command", {ESPUI.getControl(commandText)->value});
    }
}

Control *ESPUIManager::getControl(uint16_t id)
{
    return ESPUI.getControl(id);
}
