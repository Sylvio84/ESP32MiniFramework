#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include <Arduino.h>
#include <vector>
#include <map>
#include <functional>

using MainCallback = std::function<void(const String&, const String&, const std::vector<String>&)>;
using EventCallback = std::function<void(const String&, const std::vector<String>&)>;
using DebugCallBack = std::function<void(const String&, int, bool)>;

class EventManager {
public:

    void registerMainCallback(MainCallback callback);

    // Enregistre un callback pour un type d'événement spécifique
    void registerCallback(const String& eventType, EventCallback callback);

    // Déclenche un événement pour un type donné avec des paramètres
    void triggerEvent(const String& eventType, const String& event, const std::vector<String>& params);

    void registerDebugCallback(DebugCallBack callback);
    void debug(String message, int level = 0, bool displayTime = true);

private:
    // Map associant chaque type d'événement à une liste de callbacks
    std::map<String, std::vector<EventCallback>> callbacksMap;

    MainCallback mainCallback;
    DebugCallBack debugCallback;
};

#endif // EVENTMANAGER_H
