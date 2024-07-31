#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include <Arduino.h>
#include <vector>
#include <map>
#include <functional>

using EventCallback = std::function<void(const String&, const std::vector<String>&)>;

class EventManager {
public:
    // Enregistre un callback pour un type d'événement spécifique
    void registerCallback(const String& eventType, EventCallback callback);

    // Déclenche un événement pour un type donné avec des paramètres
    void triggerEvent(const String& eventType, const String& event, const std::vector<String>& params);

private:
    // Map associant chaque type d'événement à une liste de callbacks
    std::map<String, std::vector<EventCallback>> callbacksMap;
};

#endif // EVENTMANAGER_H
