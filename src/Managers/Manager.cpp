#include "Managers/Manager.h"
#include <FrameworkContext.h>
#include "Managers/EventManager.h"

// Helper method to publish events (decoupled communication)
void Manager::publishEvent(const String& type, const String& event, 
                          const std::vector<String>& params) {
    if (auto* eventMgr = context->getEventManager()) {
        eventMgr->triggerEvent(type, event, params);
    }
}

// Helper method for debug logging
void Manager::logDebug(const String& message, int level) {
    if (auto* eventMgr = context->getEventManager()) {
        eventMgr->debug("[" + getName() + "] " + message, level, true);
    }
}

// Simplified debug helper
void Manager::debug(const String& message, int level, bool displayTime) {
    if (auto* eventMgr = context->getEventManager()) {
        eventMgr->debug("[" + getName() + "] " + message, level, displayTime);
    }
}