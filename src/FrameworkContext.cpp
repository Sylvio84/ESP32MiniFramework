#include <FrameworkContext.h>
#include <Managers/Manager.h>
#include <Managers/ConfigurationManager.h>

void FrameworkContext::registerManager(Manager* manager) {
    if (manager) {
        // Store by name for type-agnostic retrieval
        managersByName[manager->getName()] = manager;
        // Also store in vector for lifecycle operations
        managers.push_back(manager);
    }
}

Manager* FrameworkContext::getManager(const String& name) const {
    auto it = managersByName.find(name);
    return (it != managersByName.end()) ? it->second : nullptr;
}