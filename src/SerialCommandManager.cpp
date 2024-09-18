#include "SerialCommandManager.h"

EventManager* SerialCommandManager::eventManager = nullptr;

void SerialCommandManager::init()
{
    Serial.begin(baudRate);
    Serial.println();
    Serial.println("SerialCommandManager initialized.");
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
                eventManager->triggerEvent("sys", "power_saving_suspend", {"Power saving suspended"});
                eventManager->triggerEvent("sys", "power_saving_resume", {"Power saving resumed", "60"});  // resume power saving after 60 seconds
                return;
            } else {
                eventManager->triggerEvent("sys", "power_saving_suspend", {});
            }
        }

        if (validate) {
            inputBuffer.trim();
            eventManager->triggerEvent("serial", "input", {inputBuffer});
            inputBuffer = "";
            eventManager->triggerEvent("sys", "power_saving_resume", {"", "60"});
        } else if (receivedChar == '\b' || receivedChar == 127) {
            if (inputBuffer.length() > 0) {
                inputBuffer.remove(inputBuffer.length() - 1);
            }
        } else {
            inputBuffer += receivedChar;
        }
    }
}