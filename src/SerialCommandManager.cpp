#include "SerialCommandManager.h"

EventManager* SerialCommandManager::eventManager = nullptr;

void SerialCommandManager::init()
{
    Serial.begin(baudRate);  // Initialisation de la communication série
    Serial.println();
    Serial.println("SerialCommandManager initialized.");
}

void SerialCommandManager::loop()
{
    handleSerialInput();  // Gérer les entrées série dans la boucle principale
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
            processCommand(inputBuffer);
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

void SerialCommandManager::processCommand(const String& input)
{
    //Serial.println("Command received: " + input);
    int nsIndex = input.indexOf(':');
    int cmdIndex = input.indexOf(' ');

    if (nsIndex == -1 || (cmdIndex != -1 && cmdIndex < nsIndex)) {
        Serial.println("Erreur: Format de commande incorrect: " + input);
        return;
    }

    // Extraction du namespace et de la commande
    String ns = input.substring(0, nsIndex);
    String command = cmdIndex == -1 ? input.substring(nsIndex + 1) : input.substring(nsIndex + 1, cmdIndex);

    // Extraction des paramètres
    String paramStr = cmdIndex == -1 ? "" : input.substring(cmdIndex + 1);
    std::vector<String> params = paramStr.isEmpty() ? std::vector<String>() : splitParameters(paramStr);

    eventManager->triggerEvent("serial", "command", {input});
    eventManager->triggerEvent(ns, "@" + command, params);
}

std::vector<String> SerialCommandManager::splitParameters(const String& paramStr)
{
    std::vector<String> params;
    String tempParam;
    bool inQuotes = false;

    for (unsigned int i = 0; i < paramStr.length(); ++i) {
        char c = paramStr[i];

        if (c == '"') {
            // Toggle the inQuotes flag
            inQuotes = !inQuotes;
            if (!inQuotes && !tempParam.isEmpty()) {
                params.push_back(tempParam);
                tempParam = "";
            }
        } else if (c == ' ' && !inQuotes) {
            // Space outside quotes indicates the end of a parameter
            if (!tempParam.isEmpty()) {
                params.push_back(tempParam);
                tempParam = "";
            }
        } else {
            // Accumulate characters for the parameter
            tempParam += c;
        }
    }

    // Push the last parameter if there's any left
    if (!tempParam.isEmpty()) {
        params.push_back(tempParam);
    }

    return params;
}