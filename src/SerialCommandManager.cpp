#include "SerialCommandManager.h"
#include <ConfigurationManager.h>
#include <EventManager.h>


// Constructor with clean dependency injection
SerialCommandManager::SerialCommandManager(FrameworkContext& ctx) : Manager(ctx), context(&ctx)
{
    auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
    baudRate = configMgr ? configMgr->getPreference("serial_speed", baudRate) : baudRate;
}


void SerialCommandManager::init()
{
    logDebug("Initializing SerialCommandManager with baud rate: " + String(baudRate), 1);
    Serial.begin(baudRate);
    Serial.println();
    Serial.println("SerialCommandManager initialized.");
    setInitialized(true);
    logDebug("SerialCommandManager initialized successfully", 1);
}

void SerialCommandManager::loop()
{
    handleSerialInput();
}

void SerialCommandManager::handleSerialInput()
{
    while (Serial.available() > 0) {
        char receivedChar = Serial.read();
        if (receivedChar == 13) {
            return;
        }

        bool validate = receivedChar == 10;

        if (inputBuffer.length() == 0) {
            if (validate) {
                context->getEventManager()->triggerEvent("sys", "power_saving_suspend", {"Power saving suspended"});
                context->getEventManager()->triggerEvent("sys", "power_saving_resume", {"Power saving resumed", "60"});  // resume power saving after 60 seconds
                return;
            } else {
                context->getEventManager()->triggerEvent("sys", "power_saving_suspend", {});
            }
        }

        if (validate) {
            inputBuffer.trim();
            context->getEventManager()->triggerEvent("serial", "input", {inputBuffer});
            inputBuffer = "";
            context->getEventManager()->triggerEvent("sys", "power_saving_resume", {"", "60"});
        } else if (receivedChar == '\b' || receivedChar == 127) {
            if (inputBuffer.length() > 0) {
                inputBuffer.remove(inputBuffer.length() - 1);
            }
        } else {
            inputBuffer += receivedChar;
        }
    }
}