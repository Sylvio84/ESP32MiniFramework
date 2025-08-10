## Architecture

### Core Structure
- **MainController Pattern**: The application follows a controller-based architecture where `MyController` extends `MainController` from the ESP32MiniFramework
- **Event-Driven**: Built on an event management system where components communicate through events (WiFi status, MQTT messages, commands)
- **Dependency Injection**: Clean DI container pattern using polymorphic service access
- **Command System**: Robust command system with namespaces, aliases, and history support

### Key Components
1. **MyController** (`src/MyController.h/cpp`): Main application logic that orchestrates all functionality
   - **Instantiates** all framework managers (defines what services are available)
   - **Registers** services with FrameworkContext
   - Handles WiFi status changes through event processing
   - Processes MQTT messages for device control
   - Manages internal LED device state

2. **FrameworkContext** (lib/ESP32MiniFramework/include/FrameworkContext.h): 
   - **Pure dependency injection container** with ZERO knowledge of concrete manager types
   - Type-agnostic storage using manager names: `getManager("WiFiManager")`
   - Core service access: `getEventManager()`
   - Manager lifecycle support: `getManagers()` for initialization loops

3. **ESP32MiniFramework** (lib/ESP32MiniFramework/): Provides base infrastructure:
   - **Manager**: Abstract base class for all framework services
   - **MainController**: Base controller that uses the Manager interface polymorphically
   - **ConfigurationManager**: Centralized configuration and preferences management
   - **CommandManager**: Command registration, execution, aliases, and history
   - **SystemManager**: System operations, power management, and control
   - **DeviceManager**: Manages hardware devices (LEDs, relays, sensors)
   - **EventManager**: Central event dispatch and handling  
   - **MQTTManager**: MQTT client implementation
   - **WiFiManager**: WiFi connection and AP management
   - **SerialCommandManager**: Serial input handling and command triggering

4. **Device Abstraction**: Hardware devices inherit from `Device` base class, allowing polymorphic device management

### Communication Flow
1. **Clean Manager Lifecycle**: `for (auto* mgr : context.getManagers()) mgr->init()`
2. **Polymorphic Event Handling**: Managers receive events via `onEvent()` interface
3. **Serial Input Processing**: 
   - SerialCommandManager reads input and triggers "serial/input" event
   - MainController processes event and calls `processInput()`
   - `processInput()` uses CommandManager to execute commands
4. **Command Execution**:
   - CommandManager parses input and finds matching command
   - Command is executed with proper source control
   - Result is returned to the user via Serial
5. Events flow through `processEvent()` for system events (WiFi, time, etc.)
6. MQTT messages are handled via `processMQTT()` with topic-based routing

### Design Benefits
- **Zero Type Dependencies**: FrameworkContext is completely decoupled from concrete manager types
- **Controller-Defined Services**: MainController decides what services exist and registers them
- **Easy Testing**: Clean interfaces and dependency injection enable easy mocking
- **Memory Efficient**: No template instantiations, uses polymorphism
- **SOLID Compliance**: Single responsibility, open/closed, dependency inversion principles
