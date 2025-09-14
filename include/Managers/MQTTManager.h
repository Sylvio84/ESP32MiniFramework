#ifndef MQTTMANAGER_H
#define MQTTMANAGER_H

#include <Arduino.h>
#include "Manager.h"
#include <PubSubClient.h>
#include <vector>
#include <map>
// #include "WiFiManager.h"
#include <FrameworkContext.h>
#ifndef DISABLE_ESPUI
#include <ESPUI.h>
#endif
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

/**
 * @brief MQTTManager - Complete MQTT client implementation with command integration
 * 
 * Comprehensive MQTT client manager that provides publish/subscribe functionality
 * with automatic reconnection, subscription management, and unified command interface.
 * 
 * ## Key Features:
 * 
 * ### Connection Management
 * - **Auto-Reconnection**: Automatic reconnection with exponential backoff
 * - **Connection Status**: Real-time connection monitoring and status reporting
 * - **WiFi Integration**: Waits for WiFi connection before attempting MQTT
 * - **Persistent Settings**: Server, port, credentials stored in NVS
 * 
 * ### Publish/Subscribe
 * - **Topic Management**: Subscribe/unsubscribe with persistent subscription list
 * - **Message Publishing**: Reliable message publishing with debug support
 * - **Payload Handling**: Proper payload parsing and string conversion
 * - **Event Integration**: MQTT messages trigger framework events
 * 
 * ### Command System Integration
 * - **Unified Commands**: All MQTT operations available via command system
 * - **Namespace**: Commands organized under `mqtt:` namespace
 * - **Aliases**: Short aliases for common operations (ms, mi, mc)
 * - **Help Integration**: Commands appear in help system with descriptions
 * 
 * ## Commands Available:
 * 
 * ### Connection Commands
 * - `mqtt:server [hostname]` - Get/set MQTT broker hostname
 * - `mqtt:port [port]` - Get/set MQTT broker port (default: 1883)
 * - `mqtt:user [username]` - Get/set MQTT username (optional)
 * - `mqtt:pass [password]` - Get/set MQTT password (optional)
 * - `mqtt:status` / `ms` - Show connection status
 * - `mqtt:connect` / `mc` - Force connection attempt
 * - `mqtt:info` / `mi` - Show detailed configuration
 * 
 * ### Topic Commands
 * - `mqtt:subscribe <topic>` - Subscribe to topic
 * - `mqtt:unsubscribe <topic>` - Unsubscribe from topic
 * - `mqtt:subscriptions` - List active subscriptions
 * - `mqtt:publish <topic> <payload>` - Publish message
 * 
 * ## Configuration:
 * ```cpp
 * mqtt:server broker.example.com
 * mqtt:port 1883
 * mqtt:user mydevice
 * mqtt:pass mypassword
 * mqtt:connect
 * ```
 * 
 * ## Event Flow:
 * 1. WiFi connects → MQTT attempts connection
 * 2. MQTT message received → "mqtt/message" event
 * 3. MainController processes → calls processMQTT()
 * 4. Message routed to appropriate handler
 * 
 * ## Architecture:
 * - **PubSubClient Integration**: Uses reliable MQTT client library
 * - **ConfigurationManager**: Persistent storage of settings
 * - **EventManager**: Event-driven message handling
 * - **CommandManager**: Unified command interface
 * - **WiFi Dependency**: Monitors WiFi status for connection management
 * 
 * ## Status Values:
 * - 0: Disabled
 * - 1: Waiting for WiFi connection
 * - 2: Keep connected (active)
 * 
 * ## Usage Notes:
 * - Requires WiFi connection before MQTT will attempt to connect
 * - Subscriptions persist across reconnections
 * - Automatic ping/keepalive every 60 seconds
 * - Exponential backoff on connection failures (1s → 60s max)
 */
class MQTTManager : public Manager
{

  public:
    // New constructor with FrameworkContext
    MQTTManager(FrameworkContext& ctx) : Manager(ctx), context(&ctx), mqttClient(wifiClient) { }
    

    // 0 = disabled, 1 = waiting wifi to connect, 2 = keep connected
    uint status = 0;

    String username = "";
    String password = "";
    String server = "";
    int port = 1883;
    bool autoReconnect = true;

    void init() override;
    void loop() override;

    bool onEvent(const String& type, const String& event, const std::vector<String>& params) override;

    void setStatus(uint status);

    bool reconnect();
    bool isConnected();
    void disconnect();

    // void onMessage(char* topic, byte* payload, unsigned int length);

    // void registerCallback(MQTTCallback callback);
    void publish(String topic, String payload, bool retain = false, bool storeIfNotConnected = false, bool enableDebug = false);
    void subscribe(String topic);
    void unsubscribe(String topic);

    void saveServer(String server);
    void savePort(int port);
    void saveUsername(String username);
    void savePassword(String password);

    String retrieveServer();
    int retrievePort();
    String retrieveUsername();
    String retrievePassword();

    String getDebugInfos();

    bool addSubscription(String topic);
    bool removeSubscription(String topic);
    std::vector<String> getSubscriptions();

    bool storePublication(String topic, String payload);
    bool removePublication(String topic);

    String getName() const override { return "MQTTManager"; }

#ifndef DISABLE_ESPUI
    void initEspUI();
    void EspUiCallback(Control* sender, int type);
#endif

  private:
    // Constants for better maintainability
    static constexpr unsigned long PING_INTERVAL_MS = 60000;
    static constexpr unsigned long MQTT_LOOP_INTERVAL_MS = 25;
    static constexpr unsigned long MAX_RECONNECT_DELAY_SEC = 60;
    static constexpr unsigned long MS_PER_SECOND = 1000;
    static constexpr unsigned int MAX_INCOMING_PACKET = MQTT_MAX_PACKET_SIZE;

    
    #ifdef ESP8266
        // Reduced buffer sizes for ESP8266 limited memory
        static constexpr size_t MAX_TOPIC_LENGTH = 48;
        static constexpr size_t MAX_VALUE_LENGTH = 128;
    #else
        // Original sizes for ESP32 and other platforms
        static constexpr size_t MAX_TOPIC_LENGTH = 64;
        static constexpr size_t MAX_VALUE_LENGTH = 256;
    #endif
    
    FrameworkContext* context;
    
    void registerCommands();

    WiFiClient wifiClient;
    PubSubClient mqttClient;

    bool connected = false;

    unsigned long lastPing = 0;
    const unsigned long pingInterval = PING_INTERVAL_MS; // 60 seconds

    std::vector<String> subscriptions;

    std::map<String, String> publications;

#ifndef DISABLE_ESPUI
    // ESPUI:
    uint16_t mqttServerInput = 0;
    uint16_t mqttPortInput = 0;
    uint16_t mqttUserInput = 0;
    uint16_t mqttPasswordInput = 0;
#endif
};

#endif
