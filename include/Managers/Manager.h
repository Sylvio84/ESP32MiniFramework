#ifndef MANAGER_H
#define MANAGER_H

#include <Arduino.h>
#include <vector>

// Forward declarations
class FrameworkContext;

/**
 * @brief Manager - Abstract base class for all framework services
 * 
 * Defines the standard interface and lifecycle for all managers in the
 * ESP32MiniFramework, ensuring consistent behavior and clean architecture.
 * 
 * ## Core Concepts:
 * 
 * ### Lifecycle Management
 * - **init()**: One-time initialization during startup (pure virtual)
 * - **loop()**: Continuous processing during runtime (optional)
 * - **cleanup()**: Resource cleanup during shutdown (optional)
 * - **isInitialized()**: Check initialization status
 * 
 * ### Communication Patterns
 * - **Event Handling**: `onEvent()` for decoupled event processing
 * - **Command Handling**: `onCommand()` for command delegation
 * - **Return true**: Indicates event/command was handled (stops propagation)
 * - **Return false**: Allows event/command to propagate to other managers
 * 
 * ### Dependency Injection
 * - **FrameworkContext**: Access to other services via context
 * - **No Direct Dependencies**: Managers don't know about concrete types
 * - **Service Discovery**: Use context->getManager("ManagerName")
 * 
 * ## Design Principles:
 * 
 * ### SOLID Compliance
 * - **Single Responsibility**: Each manager has one clear purpose
 * - **Open/Closed**: Extend via inheritance, don't modify base
 * - **Liskov Substitution**: All managers work identically in framework
 * - **Interface Segregation**: Optional methods with default implementations
 * - **Dependency Inversion**: Depend on abstractions (Manager, FrameworkContext)
 * 
 * ### Best Practices
 * - **Lightweight**: Managers should be focused and efficient
 * - **Non-Blocking**: Never block in loop() or event handlers
 * - **Stateless Logic**: Minimize internal state
 * - **No Globals**: Never use global variables or singletons
 * - **Clean Dependencies**: No circular dependencies between managers
 * 
 * ## Implementation Guidelines:
 * 
 * ```cpp
 * class MyManager : public Manager {
 * public:
 *     MyManager(FrameworkContext& ctx) : Manager(ctx) {}
 *     
 *     void init() override {
 *         // Initialize your manager
 *         setInitialized(true);
 *     }
 *     
 *     String getName() const override { 
 *         return "MyManager"; 
 *     }
 *     
 *     bool onCommand(const String& cmd, const std::vector<String>& params) override {
 *         if (cmd == "mycommand") {
 *             // Handle command
 *             return true;  // Command handled
 *         }
 *         return false;  // Not my command
 *     }
 * };
 * ```
 * 
 * ## Helper Methods:
 * - **debug()**: Unified debug output with level control
 * - **logDebug()**: Alternative debug method
 * - **publishEvent()**: Trigger events for other managers
 * - **setInitialized()**: Mark manager as ready
 */
class Manager {
public:
    /**
     * @brief Constructor with dependency injection
     * @param context Framework context for accessing other services
     */
    explicit Manager(FrameworkContext& context) : context(&context) {}
    
    /**
     * @brief Virtual destructor for proper cleanup
     */
    virtual ~Manager() = default;

    // === Core Lifecycle (Required) ===
    
    /**
     * @brief Initialize the manager
     * Called once during framework startup
     * Must be implemented by each manager
     */
    virtual void init() = 0;
    
    // === Optional Lifecycle Methods ===
    
    /**
     * @brief Main processing loop
     * Called continuously during runtime
     * Override if manager needs continuous processing
     */
    virtual void loop() {}
    
    /**
     * @brief Cleanup resources
     * Called during shutdown
     * Override if manager needs cleanup
     */
    virtual void cleanup() {}
    
    // === Event-Driven Communication (Decoupled) ===
    
    /**
     * @brief Process framework events
     * @param type Event type (e.g., "wifi", "mqtt")
     * @param event Event name (e.g., "connected", "disconnected") 
     * @param params Event parameters
     * @return true if event was handled, false otherwise
     * Override to handle relevant events
     */
    virtual bool onEvent(const String& type, const String& event, 
                        const std::vector<String>& params) { return false; }
    

    // === Status and Information ===
    
    /**
     * @brief Check if manager is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const { return initialized; }
    
    /**
     * @brief Get manager name for debugging
     * @return Manager name
     */
    virtual String getName() const = 0;

protected:
    /**
     * @brief Framework context for accessing other services
     */
    FrameworkContext* context;
    
    /**
     * @brief Initialization status
     */
    bool initialized = false;
    
    /**
     * @brief Mark manager as initialized
     * @param state Initialization state
     */
    void setInitialized(bool state = true) { 
        initialized = state; 
        //send event to indicate the manager is initialized
        publishEvent("manager", "initialized", {getName()});
    }
    
    /**
     * @brief Helper to access other services via dependency injection
     * @tparam T Service type
     * @return Pointer to service or nullptr if not found
     * Note: Use context->getService<T>() directly in manager implementations
     */
    
    /**
     * @brief Helper to publish events (decoupled communication)
     * @param type Event type
     * @param event Event name  
     * @param params Event parameters
     */
    void publishEvent(const String& type, const String& event, 
                     const std::vector<String>& params = {});
    
    /**
     * @brief Helper for debug logging
     * @param message Debug message
     * @param level Log level (0=info, 1=debug, 2=verbose)
     */
    void logDebug(const String& message, int level = 0);
    
    /**
     * @brief Simplified debug helper (same as logDebug but shorter name)
     * @param message Debug message
     * @param level Log level (0=info, 1=debug, 2=verbose)
     * @param displayTime Whether to display timestamp
     */
    void debug(const String& message, int level = 0, bool displayTime = true);
};

#endif // MANAGER_H