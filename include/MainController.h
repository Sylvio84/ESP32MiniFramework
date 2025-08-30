#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <Arduino.h>
#ifdef ESP32
#include <esp_chip_info.h>
#endif
#ifdef ESP8266
#define BEARSSL_SMALL 1
#endif

#include <FrameworkContext.h>
#include <Managers/ConfigurationManager.h>
#include <Managers/SystemManager.h>
#include <Managers/SerialManager.h>
#include <Managers/CommandManager.h>
#include <Managers/WiFiManager.h>
#include <Managers/MQTTManager.h>
#ifndef DISABLE_ESPUI
#include <Managers/ESPUIManager.h>
#endif
#include <Managers/EventManager.h>
#include <Managers/TimeManager.h>
#include <DeviceProgram.h>
#include <Tools.h>
#include <LittleFS.h>
#include <Managers/DeviceManager.h>
#include <Managers/DeviceProgramManager.h>

#define DEBUG_LOG() debugLog(__FILE__, __LINE__)

#define RELEASE_VERSION "1.2.2"
#define RELEASE_DATE "2025-08-30"

inline void debugLog( const char* file, int line)
{
    Serial.printf("(File: %s, Line: %d)\n", file, line);
}

/**
 * @brief MainController - Central orchestrator of the ESP32MiniFramework
 * 
 * Base controller class that instantiates, registers, and coordinates all
 * framework managers using polymorphic interfaces and dependency injection.
 * 
 * ## Core Responsibilities:
 * 
 * ### Manager Lifecycle
 * - **Instantiation**: Creates all framework managers in constructor
 * - **Registration**: Registers managers with FrameworkContext
 * - **Initialization**: Calls init() on all managers in correct order
 * - **Main Loop**: Calls loop() on all managers continuously
 * 
 * ### Event Processing
 * - **Event Router**: Receives events from EventManager
 * - **Manager Delegation**: Forwards events to managers via onEvent()
 * - **Special Handling**: Processes MQTT messages and serial input
 * - **Device Events**: Forwards events to device manager
 * 
 * ### Command Execution
 * - **Serial Input**: Processes commands from SerialManager
 * - **Command Parsing**: Splits input into command and parameters
 * - **CommandManager**: Delegates execution to CommandManager
 * - **Result Display**: Prints command results to Serial
 * 
 * ## Architecture:
 * 
 * ### Manager Ownership
 * ```cpp
 * protected:
 *     ConfigurationManager configManager;  // Instantiated by MainController
 *     SystemManager systemManager;         // All managers owned here
 *     CommandManager commandManager;       // etc...
 * ```
 * 
 * ### Service Registration
 * ```cpp
 * MainController() {
 *     context.registerManager(&configManager);
 *     context.registerManager(&systemManager);
 *     // Register all managers...
 * }
 * ```
 * 
 * ### Event Flow
 * 1. EventManager triggers event
 * 2. MainController::processEvent() receives it
 * 3. Delegates to managers via onEvent()
 * 4. Special cases handled directly (MQTT, serial)
 * 
 * ### Command Flow
 * 1. SerialManager triggers "serial/input" event
 * 2. MainController::processInput() parses command
 * 3. CommandManager::executeCommand() runs it
 * 4. Result printed to Serial
 * 
 * ## Virtual Methods:
 * - **init()**: Override to add custom initialization
 * - **loop()**: Override to add custom loop processing
 * - **processEvent()**: Override to handle custom events
 * - **processMQTT()**: Override for custom MQTT handling
 * - **processCommand()**: Override for custom commands
 * 
 * ## Design Patterns:
 * - **Template Method**: Base implementation with extension points
 * - **Dependency Injection**: Managers access services via context
 * - **Observer Pattern**: Event-driven communication
 * - **Command Pattern**: Decoupled command execution
 * - **Factory Pattern**: Creates and owns all managers
 * 
 * ## Usage:
 * ```cpp
 * class MyController : public MainController {
 * public:
 *     void init() override {
 *         MainController::init();  // Always call base
 *         // Add custom initialization
 *     }
 * };
 * ```
 */
class MainController
{
protected:
    FrameworkContext context;
    
    EventManager eventManager;
    ConfigurationManager configManager;
    SystemManager systemManager;
    SerialManager serialManager;
    CommandManager commandManager;
    WiFiManager wiFiManager;
    MQTTManager mqttManager;
    TimeManager timeManager;
    DeviceManager deviceManager;
    DeviceProgramManager deviceProgramManager;

    #ifndef DISABLE_ESPUI
    ESPUIManager espUIManager;
    #endif

public:
    MainController();

    virtual void init();
    virtual void loop();

#ifndef DISABLE_ESPUI
    virtual void processUI(String action, std::vector<String> params);
#endif
    bool processInput(const String input);
    virtual void processEvent(String type, String event, std::vector<String> params);
    virtual void processMQTT(String topic, String value);

    EventManager* getEventManager();
    FrameworkContext& getContext() { return context; }

    void processDebugMessage(String message, int level = 0, bool displayTime = true);
    void debug(const String& message, int level = 0, bool displayTime = true);

};

#endif
