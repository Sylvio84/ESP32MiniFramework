#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include "Manager.h"
#include <FrameworkContext.h>
#include <ESPTelnet.h>
#ifndef DISABLE_ESPUI
#include <ESPUI.h>
#endif
#include <Tools.h>
#include <vector>
#include <ArduinoJson.h>

#ifdef ESP32
#include <WiFi.h>
#include <esp_https_ota.h>
//#include <ESP32Ping.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include "ESP8266httpUpdate.h"
#endif

typedef enum {
    WM_AP_MODE_NEVER = 0,
    WM_AP_MODE_ALWAYS = 1,
    WM_AP_MODE_ON_ERROR = 2
} wm_ap_mode;


/**
 * @brief WiFiManager - Handles WiFi connection and management
 * 
 * Provides:
 * - WiFi connection management
 * - Access Point mode for configuration
 * - Telnet server for remote access
 * - Network scanning and selection
 * - Event-driven communication for WiFi events
 * @note This manager is designed to be used with a WiFi connection.
 * @note Ensure to call `loop()` periodically to maintain connection status.
 * @note The manager supports both STA and AP modes.
 * @note The manager can be extended to support more complex WiFi operations.
 */
class WiFiManager : public Manager
{
  private:
    // Constants for better maintainability
    static constexpr uint32_t CONNECTION_TIMEOUT_MS = 10000;
    static constexpr uint32_t MAX_RETRY_COUNT = 20;
    static constexpr uint32_t NETWORK_RETRY_DELAY_MS = 1000;
    static constexpr uint32_t CHECK_DELAY_MS = 100;
    static constexpr uint32_t WIFI_SCAN_TIMEOUT_MS = 5000;
    // Active reconnection backoff bounds (connection lost recovery)
    static constexpr uint32_t WIFI_RECONNECT_MIN_MS = 5000;
    static constexpr uint32_t WIFI_RECONNECT_MAX_MS = 30000;

    // Legacy constant for backward compatibility (deprecated)
    static const uint CONNECTION_TIMEOUT = CONNECTION_TIMEOUT_MS;

    ESPTelnet telnet;
    uint16_t telnetPort = 23;

    bool connected = false;
    bool keepConnected = false;
    // 0 = start, 1 = connection in progress, 2 = initial connection failed, 3 = connection lost, 4 = disconnection, 5 = Access Point, 6 = Wrong Password, 10 = connection success
    uint connectionStatus = 0;
    uint timeout = 0;
    uint checkDelay = 100;
    uint tryCount = 0;

    // Active reconnection tracking (recovery from "connection lost" + telemetry)
    unsigned long wifiReconnectCount = 0;   // total reconnection attempts since boot
    unsigned long lastReconnectAttempt = 0; // millis() of last active reconnect
    unsigned long reconnectBackoff = WIFI_RECONNECT_MIN_MS; // current backoff (5s -> 30s)

    // Configuration and state (moved to private for proper encapsulation)
    wm_ap_mode apMode = WM_AP_MODE_ON_ERROR;
    IPAddress apIP;
    String ssid = "";
    String password = "";
    
    // Saved networks functionality (private for security)
    struct SavedNetwork {
        String ssid;
        String password;
        int priority = 0;  // Higher values = higher priority
    };
    
    std::vector<SavedNetwork> savedNetworks;

    void setConnected(bool recovered = false);

#ifndef DISABLE_ESPUI
    // ESPUI:
    uint16_t ssidInput = 0;
    uint16_t passwordInput = 0;
#endif

  public:
    // New constructor with FrameworkContext
    WiFiManager(FrameworkContext& ctx) : Manager(ctx)
    {
        this->apIP = IPAddress(192, 168, 1, 249);
    }

    void init() override;
    void initConnection(bool auto_connect);
    void loop() override;

    bool onEvent(const String& type, const String& event, const std::vector<String>& params) override;
    
    void registerCommands();
    
    // Public getters for formerly public variables (proper encapsulation)
    wm_ap_mode getAPMode() const { return apMode; }
    void setAPMode(wm_ap_mode mode) { apMode = mode; }
    IPAddress getAPIP() const { return apIP; }
    void setAPIP(IPAddress ip) { apIP = ip; }
    String getCurrentSSID() const { return ssid; }
    String getCurrentPassword() const { return password; } // Note: consider removing for security
    size_t getSavedNetworkCount() const { return savedNetworks.size(); }
    unsigned long getReconnectCount() const { return wifiReconnectCount; }

    bool autoConnect();
    bool connect();
    bool keepConnection();
    void disconnect();
    void startAccessPoint(bool restart = false);
    void setupTelnet();
    void stopTelnet();
    void printTelnet(String message);
    void stopAccessPoint();
    String getStatus();
    String getSSID();
    bool isConnected();
    void saveSSID(String ssid, bool reconnect = true);
    void savePassword(String password, bool reconnect = true);
    String getDebugInfos();
    String retrieveSSID();
    String retrievePassword();
    String retrieveIP();
    String getInfo(String name);
    void scanNetworks(bool show_hidden = false);
    int getNetworks();
    int getNetworkCount(bool show_hidden = false);
    void setNetwork(int n, bool save = false);
    String getNetworkInfo(int n, String name);
    void setPowerSave(bool value);
    
    // Saved networks management
    bool saveNetwork(const String& ssid, const String& password, int priority = 0);
    bool removeNetwork(const String& ssid);
    void listSavedNetworks();
    void clearSavedNetworks();
    void loadSavedNetworks();
    void saveSavedNetworksToPrefs();
    bool connectToSavedNetwork();

    //void WiFiEvent(WiFiEvent_t event);

#ifndef DISABLE_ESPUI
    void initEspUI();
    void EspUiCallback(Control* sender, int type);
#endif

    String getName() const override { return "WiFiManager"; }

    bool otaUpdate();
};

#endif
