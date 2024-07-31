#include "../include/EventManager.h"

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
}
