#include "../include/ESPUIManager.h"

// Définition et initialisation du membre statique
EventManager *ESPUIManager::eventManager = nullptr;
uint16_t ESPUIManager::ssidInput = 0;
uint16_t ESPUIManager::passwordInput = 0;
uint16_t ESPUIManager::serialLabel = 0;
uint16_t ESPUIManager::statusLabel = 0;

uint16_t ESPUIManager::mqttServerInput = 0;
uint16_t ESPUIManager::mqttPortInput = 0;
uint16_t ESPUIManager::mqttUserInput = 0;
uint16_t ESPUIManager::mqttPasswordInput = 0;

void ESPUIManager::init()
{
    // espui.init();
    ESPUI.setVerbosity(Verbosity::Quiet);

    ESPUI.begin(hostname);
    initInfoTab();
    initDebugTab();
    initWiFiTab();
    initMQTTTab();
}

uint16_t ESPUIManager::initInfoTab()
{
    auto infoTab = ESPUI.addControl(Tab, "", "Info");
    return infoTab;
}

uint16_t ESPUIManager::initDebugTab()
{
    auto debugTab = ESPUI.addControl(Tab, "", "Debug");
    // serialLabelId = ESPUI.addControl(Label, "Serial", "Serial IN", Peterriver, maintab, textCallback);
    // statusLabelId = ESPUI.addControl(Label, "", "Serial OUT", Peterriver, serialLabelId, textCallback);
    serialLabel = ESPUI.addControl(Label, "Serial", "Serial IN", Peterriver, debugTab, ESPUIManager::textCallback);
    statusLabel = ESPUI.addControl(Label, "", "Serial OUT", Peterriver, serialLabel, ESPUIManager::textCallback);

    auto reboot = ESPUI.addControl(Button, "Reboot", "Reboot", Peterriver, debugTab, ESPUIManager::buttonCallback);
    ESPUI.setEnabled(reboot, true);

    return debugTab;
}

uint16_t ESPUIManager::initWiFiTab()
{
    auto wifiTab = ESPUI.addControl(Tab, "", "WiFi");
    ssidInput = ESPUI.addControl(Text, "SSID", "SSID", Peterriver, wifiTab, ESPUIManager::textCallback);
    passwordInput = ESPUI.addControl(Text, "Password", "Password", Peterriver, wifiTab, ESPUIManager::textCallback);

    ESPUI.setInputType(passwordInput, "password");
    ESPUI.addControl(Max, "", "32", None, ssidInput);
    ESPUI.addControl(Max, "", "64", None, passwordInput);

    auto wifisave = ESPUI.addControl(Button, "Save", "WiFiSave", Peterriver, wifiTab, ESPUIManager::buttonCallback);

    ESPUI.setEnabled(wifisave, true);

    auto connect = ESPUI.addControl(Button, "Connect", "WiFiConnect", Peterriver, wifiTab, ESPUIManager::buttonCallback);
    ESPUI.setEnabled(connect, true);
    // auto disconnect = ESPUI.addControl(Button, "Disconnect", "Disconnect", Peterriver, connect, ESPUIManager::buttonCallback);
    // auto status = ESPUI.addControl(Label, "Status", "Status", Peterriver, disconnect, ESPUIManager::textCallback);
    return wifiTab;
}

uint16_t ESPUIManager::initMQTTTab()
{
    auto mqttTab = ESPUI.addControl(Tab, "", "MQTT");
    mqttServerInput = ESPUI.addControl(Text, "Server", "hostname", Peterriver, mqttTab, ESPUIManager::textCallback);
    mqttPortInput = ESPUI.addControl(Number, "Port", "1883", Peterriver, mqttTab, ESPUIManager::textCallback);
    mqttUserInput = ESPUI.addControl(Text, "User", "User", Peterriver, mqttTab, ESPUIManager::textCallback);
    mqttPasswordInput = ESPUI.addControl(Text, "Password", "Password", Peterriver, mqttTab, ESPUIManager::textCallback);

    ESPUI.setInputType(mqttPasswordInput, "password");

    auto mqttSave = ESPUI.addControl(Button, "Save", "Save", Peterriver, mqttTab, ESPUIManager::buttonCallback);

    ESPUI.setEnabled(mqttSave, true);

    auto mqttReconnect = ESPUI.addControl(Button, "Reconnect", "MQTTReconnect", Peterriver, mqttTab, ESPUIManager::buttonCallback);
    ESPUI.setEnabled(mqttReconnect, true);

    return mqttTab;
}

void ESPUIManager::processCommand(String command)
{
}

void ESPUIManager::print(uint8_t labelId, String text)
{
    ESPUI.print(labelId, text);
}

void ESPUIManager::textCallback(Control *sender, int type)
{
    Serial.println("Text callback: sender.value = " + sender->value + " sender.id = " + sender->id + " sender.type = " + sender->type + "  / type = " + String(type));
    /*Serial.println(sender->value);
    std::vector<String> params;
    params.push_back(sender->value);
    eventManager->triggerEvent("ESPUI", "InputChange", params);*/
}

void ESPUIManager::buttonCallback(Control *sender, int type)
{
    Serial.println("Button callback: sender.value = " + sender->value + " sender.id = " + sender->id + " sender.type = " + sender->type + "  / type = " + String(type));
    if (type == B_DOWN)
    {
        return;
    }
    if (sender->value == "WiFiConnect")
    {
        eventManager->triggerEvent("ESPUI", "WiFiConnect", {});
    }
    else if (sender->value == "WiFiSave")
    {
        std::vector<String> params1;
        params1.push_back(ESPUI.getControl(ssidInput)->value);
        eventManager->triggerEvent("ESPUI", "WiFiSaveSSID", params1);

        std::vector<String> params2;
        params2.push_back(ESPUI.getControl(passwordInput)->value);
        eventManager->triggerEvent("ESPUI", "WiFiSavePassword", params2);
    }
    else if (sender->value == "MQTTSave")
    {
        std::vector<String> params1;
        params1.push_back(ESPUI.getControl(mqttServerInput)->value);
        eventManager->triggerEvent("ESPUI", "MQTTSaveServer", params1);

        std::vector<String> params2;
        params2.push_back(ESPUI.getControl(mqttPortInput)->value);
        eventManager->triggerEvent("ESPUI", "MQTTSavePort", params2);

        std::vector<String> params3;
        params3.push_back(ESPUI.getControl(mqttUserInput)->value);
        eventManager->triggerEvent("ESPUI", "MQTTSaveUser", params3);

        std::vector<String> params4;
        params4.push_back(ESPUI.getControl(mqttPasswordInput)->value);
        eventManager->triggerEvent("ESPUI", "MQTTSavePassword", params4);
    }
    else if (sender->value == "MQTTReconnect")
    {
        eventManager->triggerEvent("ESPUI", "MQTTReconnect", {});
    }
    else if (sender->value == "Reboot")
    {
        eventManager->triggerEvent("ESPUI", "Reboot", {});
    }
}

Control *ESPUIManager::getControl(uint16_t id)
{
    return ESPUI.getControl(id);
}

/*
ESPUI ESPUIManager::getEspui()
{
    return espui;
}
*/