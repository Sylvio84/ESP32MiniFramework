#include "../include/EventManager.h"

void EventManager::registerMainCallback(MainCallback callback) {
    mainCallback = callback;
}

void EventManager::registerCallback(const String& eventType, EventCallback callback) {
    callbacksMap[eventType].push_back(callback);
}

void EventManager::triggerEvent(const String& eventType, const String& event, const std::vector<String>& params) {
    auto it = callbacksMap.find(eventType);
    if (it != callbacksMap.end()) {
        for (auto& callback : it->second) {
            callback(event, params);
        }
    }
    if (mainCallback) {
        mainCallback(eventType, event, params);
    }
}

void EventManager::registerDebugCallback(DebugCallBack callback) {
    debugCallback = callback;
}

void EventManager::debug(String message, int level, bool displayTime)
{
    if (debugCallback) {
        debugCallback(message, level, displayTime);
    }
}