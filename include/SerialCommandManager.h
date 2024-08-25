#ifndef SERIAL_COMMAND_MANAGER_H
#define SERIAL_COMMAND_MANAGER_H

#include <Arduino.h>
#include <map>
#include <vector>
#include <functional>
#include <Configuration.h>
#include <EventManager.h>

class SerialCommandManager
{
public:
    SerialCommandManager(Configuration& config, EventManager &eventMgr) : config(config)
    {
        baudRate = config.getPreference("serial_speed", baudRate);
        if (eventManager == nullptr)
        {
            eventManager = &eventMgr;
        }
    }

    // Méthodes publiques
    void init();                                                                         // Initialise la communication série
    void loop();                                                                         // Gère les commandes dans la boucle principale

private:
    // Attributs
    int baudRate = 115200; // Vitesse de communication série
    Configuration &config;

    // Méthodes privées
    void handleSerialInput();                                    // Gère l'entrée série
    void processCommand(const String &input);                    // Traite la commande reçue
    std::vector<String> splitParameters(const String &paramStr); // Divise la chaîne de paramètres

    static EventManager *eventManager; // Pointeur vers EventManager
};

#endif // SERIAL_COMMAND_MANAGER_H
