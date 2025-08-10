# Development Guide

## Core Principles

### SOLID Architecture
The ESP32MiniFramework strictly follows SOLID principles:
- **S**ingle Responsibility: Each manager handles one concern
- **O**pen/Closed: Extend through inheritance, not modification
- **L**iskov Substitution: All managers are interchangeable through base interface
- **I**nterface Segregation: Clean, focused interfaces for each component
- **D**ependency Inversion: Depend on abstractions (Manager), not concretions

### C++11 Compliance
- Use modern C++11 features (auto, nullptr, range-based for)
- NO C++14/17/20 features (ESP8266 compatibility)
- Prefer stack allocation over heap when possible
- Use String class for Arduino compatibility

## Manager Architecture

### 1. Base Manager Class
All framework services extend the `Manager` base class:

```cpp
class Manager {
public:
    virtual bool init() = 0;                    // Initialize the manager
    virtual void loop() = 0;                    // Main loop processing
    virtual bool onEvent(Event& event);         // Event handling (return true if handled)
    virtual bool onCommand(String cmd, std::vector<String> params); // Command handling
    
protected:
    void debug(String message, int level = 1);  // Consistent debug logging
};
```

### 2. Manager Lifecycle
```cpp
// Initialization order matters!
1. ConfigurationManager   // Load settings first
2. SystemManager         // System control
3. WiFiManager          // Network connectivity
4. MQTTManager         // After network is available
5. DeviceManager       // Hardware devices
6. Other managers...   // Application-specific
```

### 3. Core Managers

| Manager | Responsibility | Key Methods |
|---------|---------------|-------------|
| **ConfigurationManager** | Settings & preferences | `getValue()`, `setValue()`, `save()` |
| **SystemManager** | System operations | `restart()`, `getUptime()`, `getVersion()` |
| **CommandManager** | Command execution | `execute()`, `registerCommand()`, `addAlias()` |
| **SerialCommandManager** | Serial I/O | `processInput()`, `readSerial()` |
| **WiFiManager** | Network connectivity | `connect()`, `scan()`, `getStatus()` |
| **MQTTManager** | MQTT communication | `publish()`, `subscribe()`, `onMessage()` |
| **DeviceManager** | Hardware control | `registerDevice()`, `getDevice()` |
| **EventManager** | Event dispatch | `emit()`, `on()`, `off()` |

## Design Patterns

### 1. Service Registration
```cpp
void MyController::setup() {
    // Instantiate managers
    static ConfigurationManager configMgr;
    static SystemManager systemMgr;
    static WiFiManager wifiMgr;
    
    // Register with context
    context.registerManager(&configMgr);
    context.registerManager(&systemMgr);
    context.registerManager(&wifiMgr);
    
    // Initialize all
    for (auto* mgr : context.getManagers()) {
        mgr->init();
    }
}
```

### 2. Polymorphic Service Access
```cpp
// Safe casting with type checking
auto* wifiMgr = static_cast<WiFiManager*>(
    context.getManager("WiFiManager")
);
if (wifiMgr) {
    wifiMgr->connect(ssid, password);
}
```

### 3. Event Handling
```cpp
bool WiFiManager::onEvent(Event& event) {
    if (event.type == "network/disconnect") {
        handleDisconnect();
        return true;  // Event consumed
    }
    return false;     // Event not handled
}
```

### 4. Command Processing
```cpp
bool SystemManager::onCommand(String cmd, std::vector<String> params) {
    if (cmd == "sys:restart") {
        ESP.restart();
        return true;  // Command handled
    }
    return false;     // Pass to next manager
}
```

## Coding Standards

### Naming Conventions
- **Classes**: PascalCase (`WiFiManager`, `DeviceManager`)
- **Methods**: camelCase (`getValue()`, `processEvent()`)
- **Members**: m_prefix (`m_connected`, `m_lastError`)
- **Constants**: UPPER_SNAKE (`MAX_RETRIES`, `DEFAULT_TIMEOUT`)
- **Namespaces**: lowercase (`framework`, `devices`)


### Error Handling
```cpp
// Use return codes for operations
enum class Result {
    SUCCESS = 0,
    ERROR_TIMEOUT = -1,
    ERROR_INVALID_PARAM = -2,
    ERROR_NOT_CONNECTED = -3
};

// Provide clear error messages
if (!wifi.isConnected()) {
    debug("WiFi not connected", DEBUG_ERROR);
    return Result::ERROR_NOT_CONNECTED;
}
```

### Debug Logging
```cpp
// Use consistent debug levels
enum DebugLevel {
    DEBUG_ERROR = 0,   // Always shown
    DEBUG_INFO = 1,    // Important info
    DEBUG_VERBOSE = 2, // Detailed info
    DEBUG_TRACE = 3    // Everything
};

// Example usage
debug("Connecting to " + ssid, DEBUG_INFO);
debug("RSSI: " + String(rssi), DEBUG_VERBOSE);
```

## Creating New Managers

### Manager Template
```cpp
// MyCustomManager.h
class MyCustomManager : public Manager {
public:
    MyCustomManager();
    bool init() override;
    void loop() override;
    bool onEvent(Event& event) override;
    bool onCommand(String cmd, std::vector<String> params) override;
    
private:
    void doSomething();
    bool m_initialized = false;
};

// MyCustomManager.cpp
bool MyCustomManager::init() {
    debug("Initializing MyCustomManager", DEBUG_INFO);
    m_initialized = true;
    return true;
}

void MyCustomManager::loop() {
    if (!m_initialized) return;
    // Process periodic tasks
}
```

### Registration in Controller
```cpp
// In MyController.cpp
void MyController::setup() {
    static MyCustomManager customMgr;
    context.registerManager(&customMgr);
    // ... register other managers
}
```

## Testing Guidelines

### Unit Testing
- Test each manager independently
- Mock dependencies using interfaces
- Verify event handling and command processing

### Integration Testing
- Test manager interactions
- Verify event flow between components
- Test command delegation chain

### Hardware Testing
- Use conditional compilation for hardware-specific code
- Provide software mocks for development
- Test on both ESP32 and ESP8266

## Performance Considerations

### Loop Optimization
```cpp
void Manager::loop() {
    // Use time-based execution
    static unsigned long lastRun = 0;
    if (millis() - lastRun < 100) return;  // Run every 100ms
    lastRun = millis();
    
    // Do work here
}
```

## Common Pitfalls

### ❌ DON'T
- Don't use delay() in managers (blocks entire system)
- Don't allocate large buffers on stack
- Don't ignore return values from init()
- Don't use raw pointers without checking nullptr

### ✅ DO
- Use millis() for non-blocking delays
- Allocate large buffers globally or on heap
- Always check initialization success
- Validate pointers before dereferencing

## Quick Reference

### Adding a New Feature
1. Create manager class extending `Manager`
2. Implement required virtual methods
3. Register in `MyController::setup()`
4. Add commands to handle user input
5. Emit events for state changes
6. Document in architecture.md

### Debugging Tips
- Enable verbose logging: `sys:debuglevel 3`
- Monitor heap: `sys:info` shows free memory
- Use Serial plotter for sensor data
- Check event flow with trace logging

