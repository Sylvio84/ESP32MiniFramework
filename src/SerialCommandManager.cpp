#include "SerialCommandManager.h"

EventManager *SerialCommandManager::eventManager = nullptr;

void SerialCommandManager::init()
{
    Serial.begin(baudRate); // Initialisation de la communication série
    Serial.println();
    Serial.println("SerialCommandManager initialized.");
}

void SerialCommandManager::loop()
{
    handleSerialInput(); // Gérer les entrées série dans la boucle principale
}

void SerialCommandManager::handleSerialInput()
{
    if (Serial.available() > 0)
    {
        String input = Serial.readStringUntil('\n');
        input.trim();
        processCommand(input);
    }
}

void SerialCommandManager::processCommand(const String &input)
{
    //Serial.println("Command received: " + input);
    int nsIndex = input.indexOf(':');
    int cmdIndex = input.indexOf(' ');

    if (nsIndex == -1 || (cmdIndex != -1 && cmdIndex < nsIndex))
    {
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

std::vector<String> SerialCommandManager::splitParameters(const String& paramStr) {
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