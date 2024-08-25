#include "../include/ESPUIManager.h"

// Définition et initialisation du membre statique
EventManager *ESPUIManager::eventManager = nullptr;

void ESPUIManager::init()
{
    Serial.println("ESPUIManager init...");
    ESPUI.setVerbosity(Verbosity::Quiet);

    ESPUI.begin(hostname);
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
    serialLabel = ESPUI.addControl(Label, "Serial", "Serial IN", Peterriver, debugTab, callback);
    statusLabel = ESPUI.addControl(Label, "", "Serial OUT", Peterriver, serialLabel, callback);

    auto reboot = ESPUI.addControl(Button, "Reboot", "Reboot", Peterriver, debugTab, callback);
    ESPUI.setEnabled(reboot, true);

    return debugTab;
}

void ESPUIManager::processCommand(String command)
{
}

void ESPUIManager::print(uint8_t labelId, String text)
{
    ESPUI.print(labelId, text);
}

void ESPUIManager::EspUiCallback(Control *sender, int type)
{
    Serial.println("Button callback: sender.value = " + sender->value + " sender.id = " + sender->id + " sender.type = " + sender->type + "  / type = " + String(type));
    if (type == B_DOWN)
    {
        return;
    }

    if (sender->value == "Reboot")
    {
        eventManager->triggerEvent("ESPUI", "Reboot", {});
    }
}

Control *ESPUIManager::getControl(uint16_t id)
{
    return ESPUI.getControl(id);
}
