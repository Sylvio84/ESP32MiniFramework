#ifndef FRAMEWORKCONTEXT_H
#define FRAMEWORKCONTEXT_H

#include <Arduino.h>
#include <vector>
#include <map>

// Forward declarations - only what we actually need
class Manager;
class ConfigurationManager;
class EventManager;

/**
 * @brief FrameworkContext - Pure dependency injection container
 * 
 * Type-agnostic service container that manages the lifecycle and access to
 * all framework managers without knowing their concrete types.
 * 
 * ## Key Principles:
 * 
 * ### SOLID Compliance
 * - **Single Responsibility**: Pure service container functionality only
 * - **Open/Closed**: Add new managers without modifying container code
 * - **Dependency Inversion**: Depends only on Manager abstraction
 * - **Interface Segregation**: Minimal, focused interface
 * - **Liskov Substitution**: Any Manager subclass works identically
 * 
 * ### Type Agnosticism
 * - **Zero Type Dependencies**: No knowledge of concrete manager types
 * - **Name-Based Retrieval**: Managers accessed by string name
 * - **Polymorphic Storage**: All managers stored as Manager* base class
 * - **Runtime Casting**: Clients cast to concrete types as needed
 * 
 * ## Architecture:
 * 
 * ### Service Registration
 * ```cpp
 * context.registerManager(&wifiManager);     // Any Manager subclass
 * context.registerService(&eventManager);    // Core services
 * ```
 * 
 * ### Service Retrieval
 * ```cpp
 * // Type-safe casting by client
 * auto* wifi = static_cast<WiFiManager*>(context.getManager("WiFiManager"));
 * auto* events = context.getEventManager();  // Direct core service access
 * ```
 * 
 * ### Lifecycle Management
 * ```cpp
 * // Iterate all managers for initialization
 * for (auto* mgr : context.getManagers()) {
 *     mgr->init();
 * }
 * ```
 * 
 * ## Design Benefits:
 * - **Decoupling**: Container knows nothing about manager implementations
 * - **Flexibility**: New managers added without framework changes
 * - **Testability**: Easy to mock managers for unit testing
 * - **Memory Efficient**: No template instantiations
 * - **Controller-Defined**: MainController decides what services exist
 * 
 * ## Integration:
 * - MainController creates and registers all managers
 * - Managers use context to access other services
 * - No circular dependencies between managers
 * - Clean separation of concerns
 */
class FrameworkContext {
public:
    // === Service Registration ===
    
    /**
     * @brief Register core services (don't extend Manager)
     */
    void registerService(EventManager* service) { eventManager = service; }
    
    /**
     * @brief Register any manager (completely type-agnostic)
     * @param manager Manager instance to register
     * 
     * Stores manager by its getName() for type-safe retrieval.
     * FrameworkContext has no knowledge of concrete manager types.
     */
    void registerManager(Manager* manager);
    
    /**
     * @brief Unified registration for any Manager-derived type
     * @tparam T Manager type (must extend Manager)
     * @param manager Manager instance to register
     */
    template<typename T>
    void registerService(T* manager) {
        registerManager(manager);  // Implicit conversion to Manager*
    }

    // === Service Access (Dependency Injection) ===
    
    // No more getService<T>() - use getManager(name) for polymorphic access
    
    /**
     * @brief Get manager by name (polymorphic access)
     * @param name Manager name (from getName())
     * @return Manager instance or nullptr
     */
    Manager* getManager(const String& name) const;
    
    // === Manager Lifecycle Support ===
    
    /**
     * @brief Get all registered managers for lifecycle operations
     * @return Vector of all managers
     */
    const std::vector<Manager*>& getManagers() const { return managers; }
    
    // === Core Services Access ===
    
    /**
     * @brief Get EventManager service (doesn't extend Manager) 
     */
    EventManager* getEventManager() const { return eventManager; }
    
    // Legacy getConfiguration() removed - use getManager("ConfigurationManager") directly

private:
    // Core services (don't extend Manager)
    EventManager* eventManager = nullptr;
    
    // Manager storage - completely type-agnostic
    std::map<String, Manager*> managersByName;  // Storage by manager name
    std::vector<Manager*> managers;             // Lifecycle operations
};


#endif