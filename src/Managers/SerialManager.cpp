#include "Managers/SerialManager.h"
#include "Managers/ConfigurationManager.h"
#include "Managers/EventManager.h"


// Constructor with clean dependency injection
SerialManager::SerialManager(FrameworkContext& ctx) : Manager(ctx), context(&ctx)
{
    auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
    baudRate = configMgr ? configMgr->getPreference("serial_speed", baudRate) : baudRate;
}


void SerialManager::init()
{
    logDebug("Initializing SerialManager with baud rate: " + String(baudRate), 1);
    Serial.begin(baudRate);
    Serial.println();
    setInitialized(true);
    logDebug("SerialManager initialized successfully", 1);
}

void SerialManager::loop()
{
    handleInput();
}

void SerialManager::handleInput()
{
    while (Serial.available() > 0) {
        char receivedChar = Serial.read();
        // Handle special characters 13 = CR, 10 = LF
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
            String cmd = inputBuffer;
            cmd.trim();
            inputBuffer = "";
            context->getEventManager()->triggerEvent("serial", "input", {cmd});
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

void SerialManager::addToInputBuffer(const String input, bool resetBuffer)
{
    if (resetBuffer) {
        inputBuffer = input;
    } else {
        inputBuffer += input;
    }
}

void SerialManager::output(const String& output)
{
    Serial.print(output);
}
