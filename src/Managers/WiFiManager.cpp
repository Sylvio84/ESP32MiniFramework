#include <ArduinoJson.h>
#include "Managers/CommandManager.h"
#include "Managers/ConfigurationManager.h"
#include "Managers/EventManager.h"
#include "Managers/MQTTManager.h"
#include "Managers/TimeManager.h"
#include "Managers/WiFiManager.h"
#include <algorithm>

void WiFiManager::init()
{

    logDebug("Init WiFiManager", 1);

    // Load saved networks first
    loadSavedNetworks();

    // Get legacy SSID/password for backward compatibility
    retrieveSSID();
    retrievePassword();

    initConnection(true);

    // Register WiFi commands with CommandManager
    registerCommands();

    setInitialized(true);
}

void WiFiManager::initConnection(bool auto_connect = true)
{

    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    apMode = static_cast<wm_ap_mode>(configMgr ? configMgr->getPreference("ap_mode", 2) : 2);

    // Connection logic - unified system
    if (auto_connect) {
        bool connected = false;

        // First try saved networks (new system)
        if (!savedNetworks.empty()) {
            connected = connectToSavedNetwork();
            if (connected) {
                logDebug("Connected using saved networks", 1);
            }
        }

        // If no saved networks or connection failed, try legacy SSID (backward compatibility)
        // But only if it's not already in saved networks
        if (!connected && this->ssid.length() > 0) {
            bool legacyAlreadyTried = false;
            for (const auto& network : savedNetworks) {
                if (network.ssid == this->ssid) {
                    legacyAlreadyTried = true;
                    break;
                }
            }

            if (!legacyAlreadyTried) {
                logDebug("Trying legacy SSID: " + this->ssid, 1);
                connected = this->connect();
            } else {
                logDebug("Legacy SSID already tried in saved networks: " + this->ssid, 2);
            }
        }

        // If nothing worked, start Access Point
        if (!connected) {
            logDebug("No connection possible, starting Access Point", 0);
            startAccessPoint();
        }
    } else {
        // Auto-connect disabled - just start AP if no networks configured
        if (savedNetworks.empty() && (this->ssid.length() == 0)) {
            logDebug("No networks configured, starting Access Point", 0);
            startAccessPoint();
        }
    }
}

void WiFiManager::loop()
{
    telnet.loop();
    if (!this->ssid.length()) {
        return;
    }

    static unsigned long lastMillis = 0;
    unsigned long currentMillis = millis();

    if (connectionStatus == 0) {
        this->connect();
        lastMillis = currentMillis;
        connectionStatus = 1;
    }

    unsigned long wait = (tryCount + 1) * checkDelay;

    if (currentMillis - lastMillis >= wait) {
        if (connectionStatus < 10) {
            if (WiFi.status() != WL_CONNECTED) {
                this->connected = false;
#ifdef ESP8266
                if (WiFi.status() == WL_WRONG_PASSWORD) {
                    if (connectionStatus != 6) {
                        connectionStatus = 6;
                        logDebug("WiFi: Wrong password", 0);
                        context->getEventManager()->triggerEvent("wifi", "wrong_password", {});
                        this->startAccessPoint();
                    }
                }
#endif
                tryCount++;
                std::vector<String> params;
                params.push_back(String(tryCount));
                context->getEventManager()->triggerEvent("wifi", "in_progress", params);
                if (tryCount >= MAX_RETRY_COUNT) {
                    tryCount = 0;
                    if (keepConnected) {
                        // Try all saved networks in priority order
                        if (connectToSavedNetwork()) {
                            logDebug("WiFi: Connected to saved network", 1);
                        } else {
                            logDebug("WiFi: Connection failed, retrying", 1);
                        }
                    } else {
                        if (connectionStatus != 6) {
                            connectionStatus = 2;
                            logDebug("WiFi: Connection failed", 1);
                            context->getEventManager()->triggerEvent("wifi", "failed", params);

                            // Try all saved networks before starting AP
                            if (connectToSavedNetwork()) {
                                logDebug("WiFi: Connected to saved network", 1);
                            } else {
                                // if wifi mode is AP, restart AP
                                this->startAccessPoint();
                            }
                        }
                    }
                }
                logDebug("WiFi: connection in progress #" + String(tryCount) + "...", 2);
            } else {
                setConnected();
            }
        } else if (connectionStatus == 10)  // Connected
        {
            if (WiFi.status() != WL_CONNECTED) {
                connectionStatus = 3;
                this->connected = false;
                logDebug("WiFi: Connection lost", 1);
                context->getEventManager()->triggerEvent("wifi", "lost", {});
            }
        } else if (connectionStatus == 3)  // Connection lost
        {
            if (WiFi.status() == WL_CONNECTED) {
                setConnected(true);
            }
        }
        lastMillis = currentMillis;
    }
}

bool WiFiManager::onEvent(const String& type, const String& event, const std::vector<String>& params)
{
    if (type == "wifi") {
        if (event == "connected" || event == "recovered") {
            // Update time when WiFi connects
            auto* timeMgr = static_cast<TimeManager*>(context->getManager("TimeManager"));
            if (timeMgr) {
                timeMgr->update();
            }

            // Update MQTT status
            auto* mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
            if (mqttMgr) {
                mqttMgr->setStatus(2);
            }

            debug("Connected to WiFi: " + (params.size() > 0 ? params[0] : "unknown"), 1);
            if (params.size() > 1) {
                debug("IP address: " + params[1], 1);
            }
            return false;  // Don't block other managers from handling this event

        } else if (event == "ap_started") {
            debug("WiFi Access Point started", 1);
            // ESPUI.begin(); // Could be handled here if needed
            return false;  // Don't block other managers from handling this event

        } else if (event == "disconnected" || event == "lost") {
            // Update MQTT status when WiFi disconnects
            auto* mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
            if (mqttMgr) {
                mqttMgr->setStatus(1);
            }

            debug("WiFi disconnected", 1);
            return false;  // Don't block other managers from handling this event

        } else if (event.startsWith("@")) {
            // Handle WiFi commands via events
            return onCommand(event.substring(1), params);
        }
    }

    return false;  // Event not handled
}

bool WiFiManager::onCommand(const String& command, const std::vector<String>& params)
{
    // Legacy command system - most commands migrated to registerCommands()
    // Keep only non-duplicated commands here

    logDebug("Processing legacy WiFi command: " + command, 3);

    if (command == "debug") {
        logDebug("Debug infos:", 0);
        logDebug("SSID: " + retrieveSSID() + "\nPassword: " + retrievePassword(), 0);
    } else if (command == "keep") {
        if (keepConnection()) {
            logDebug("Keep connection: ON", 0);
        } else {
            logDebug("Keep connection: OFF", 0);
        }
    } else if (command == "telnet") {
        setupTelnet();
#ifdef ESP32
    } else if (command == "ping") {
        if (params.size() > 0) {
            logDebug("Ping: " + params[0], 0);
            // NOTE: Ping functionality currently disabled due to library dependency
            logDebug("Ping functionality not available", 1);
        } else {
            logDebug("Missing IP address", 1);
        }
#endif
    } else {
        return false;  // Command not handled by legacy system
    }
    return true;
}

bool WiFiManager::autoConnect()
{
    if (WiFi.status() != WL_CONNECTED) {
        logDebug("WiFi AutoConnect...", 0);
        if (!this->ssid.length()) {
            logDebug("No SSID, starting access point", 0);
            this->startAccessPoint();
            return false;
        } else {
            connectionStatus = 0;
            connect();
            return WiFi.status() == WL_CONNECTED;
        }
    } else {
        return true;
    }
}

bool WiFiManager::connect()
{
    connectionStatus = 1;
    if (!this->ssid.length()) {
        logDebug("No SSID", 1);
        return false;
    }
    logDebug("Connecting to: " + this->ssid + "...", 0);

    WiFiMode_t mode = WIFI_STA;
    if (apMode == WM_AP_MODE_ALWAYS) {
        mode = WIFI_AP_STA;
    }
    if (WiFi.getMode() != mode) {
        WiFi.mode(mode);
    }

    //WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, IPAddress(8, 8, 8, 8));

    WiFi.begin(this->ssid.c_str(), this->password.c_str());

    // Wait for connection with timeout (10 seconds)
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < CONNECTION_TIMEOUT) {
        delay(100);
        if ((millis() - startTime) % 1000 == 0) {
            logDebug(".", 0);
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        setConnected();
        return true;
    } else {
        logDebug("Connection timeout for: " + this->ssid, 1);
        return false;
    }
}

void WiFiManager::setConnected(bool recovered)
{
    connected = true;
    keepConnected = true;
    connectionStatus = 10;
    tryCount = 0;
    std::vector<String> params;
    params.push_back(WiFi.SSID());
    params.push_back(WiFi.localIP().toString());
    context->getEventManager()->triggerEvent("wifi", recovered ? "recovered" : "connected", params);
    logDebug("Connected to WiFi: " + WiFi.SSID() + " (" + WiFi.localIP().toString() + ")", 0);
}

void WiFiManager::disconnect()
{
    WiFi.disconnect();
    this->connected = false;
    connectionStatus = 4;
    context->getEventManager()->triggerEvent("wifi", "disconnected", {});
}

void WiFiManager::startAccessPoint(bool restart)
{
    if (apMode == WM_AP_MODE_NEVER) {
        logDebug("Hotspot disabled", 1);
        return;
    }
    if (!restart && (WiFi.getMode() == WIFI_AP_STA || WiFi.getMode() == WIFI_AP)) {
        logDebug("Hotspot already started", 1);
        return;
    }

    //disconnect();
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    String hostname = configMgr ? configMgr->getHostname() : "ESP32";
    logDebug("Creating Hotspot: " + hostname, 0);
    logDebug("IP Address: " + this->apIP.toString(), 0);
    WiFi.mode(WIFI_AP_STA);
    delay(100);
    WiFi.softAPConfig(this->apIP, this->apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(hostname.c_str());
    setupTelnet();
    //connectionStatus = 5;
    //connected = false;
    context->getEventManager()->triggerEvent("wifi", "ap_started", {});
}

void WiFiManager::setupTelnet()
{
    // passing on functions for various telnet events
    telnet.onConnect([this](const String& str) {
        logDebug("Telnet connected", 2);
        context->getEventManager()->triggerEvent("telnet", "connected", {});
    });

    telnet.onConnectionAttempt([this](const String& str) {
        logDebug("Telnet connection attempt", 2);
        context->getEventManager()->triggerEvent("telnet", "connection_attempt", {str});
    });
    telnet.onReconnect([this](const String& str) {
        logDebug("Telnet reconnected", 2);
        context->getEventManager()->triggerEvent("telnet", "reconnected", {});
    });
    telnet.onDisconnect([this](const String& str) {
        logDebug("Telnet disconnected", 2);
        context->getEventManager()->triggerEvent("telnet", "disconnected", {});
    });
    telnet.onInputReceived([this](const String& str) {
        logDebug("Telnet input received: " + str, 2);
        context->getEventManager()->triggerEvent("telnet", "input", {str});
    });

    if (telnet.begin(telnetPort)) {
        logDebug("Telnet server started on port " + String(telnetPort), 1);
        context->getEventManager()->triggerEvent("telnet", "started", {String(telnetPort)});
    } else {
        logDebug("Telnet server could not start", 1);
    }
}

void WiFiManager::stopTelnet()
{
    telnet.stop();
    context->getEventManager()->triggerEvent("telnet", "stopped", {});
    logDebug("Telnet stopped", 0);
}

void WiFiManager::printTelnet(String message)
{
    if (!telnet.isConnected()) {
        return;
    }
    telnet.print(message);
}

void WiFiManager::stopAccessPoint()
{
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    context->getEventManager()->triggerEvent("wifi", "ap_stopped", {});
    logDebug("Hotspot stopped", 0);
}

String WiFiManager::getSSID()
{
    // Return currently connected SSID if connected, otherwise configured SSID
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.SSID();
    }
    return this->ssid;
}

bool WiFiManager::isConnected()
{
    // Check both internal state and actual WiFi status for consistency
    bool actuallyConnected = (WiFi.status() == WL_CONNECTED);
    if (this->connected != actuallyConnected) {
        // Update internal state if inconsistent
        this->connected = actuallyConnected;
        logDebug("WiFi connection state synchronized: " + String(actuallyConnected), 2);
    }
    return this->connected;
}

void WiFiManager::saveSSID(String ssid, bool reconnect)
{
    this->ssid = ssid;
    logDebug("New WiFi SSID: " + this->ssid, 1);

    // Save to legacy preferences for backward compatibility
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    if (configMgr) {
        configMgr->setPreference("wf_ssid", ssid);
        logDebug("SSID saved to legacy preferences", 2);
    }

    // If we have both SSID and password, save to unified system with highest priority
    if (!this->ssid.isEmpty() && !this->password.isEmpty()) {
        if (saveNetwork(this->ssid, this->password, 1000)) {
            logDebug("Network saved to unified system with priority 1000", 2);
        }
    }

    if (reconnect) {
        disconnect();
        connect();
    }
}

void WiFiManager::savePassword(String password, bool reconnect)
{
    this->password = password;
    logDebug("New WiFi password: " + this->password, 1);

    // Save to legacy preferences for backward compatibility
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    if (configMgr) {
        configMgr->setPreference("wf_pass", password);
        logDebug("Password saved to legacy preferences", 2);
    }

    // If we have both SSID and password, save to unified system with highest priority
    if (!this->ssid.isEmpty() && !this->password.isEmpty()) {
        if (saveNetwork(this->ssid, this->password, 1000)) {
            logDebug("Network saved to unified system with priority 1000", 2);
        }
    }

    if (reconnect) {
        disconnect();
        connect();
    }
}

String WiFiManager::getDebugInfos()
{
    return "SSID: " + retrieveSSID() + "\nPassword: " + retrievePassword();
}


/**
 * @deprecated
 */
String WiFiManager::retrieveSSID()
{
    // Use ConfigurationManager instead of old Configuration
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    if (configMgr) {
        ssid = configMgr->getPreference("wf_ssid", ssid);
        logDebug("SSID retrieved from preferences: " + ssid, 3);
    } else {
        logDebug("ConfigurationManager not available for retrieving SSID", 1);
    }
    return ssid;
}

/**
 * @deprecated
 */
String WiFiManager::retrievePassword()
{
    // Use ConfigurationManager instead of old Configuration
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    if (configMgr) {
        password = configMgr->getPreference("wf_pass", password);
        logDebug("Password retrieved from preferences", 3);
    } else {
        logDebug("ConfigurationManager not available for retrieving password", 1);
    }
    return password;
}

String WiFiManager::retrieveIP()
{
    return WiFi.localIP().toString();
}

bool WiFiManager::keepConnection()
{
    keepConnected = !keepConnected;
    return keepConnected;
}

String WiFiManager::getStatus()
{
    switch (connectionStatus) {
        case 0:
            return "Init WiFi";
        case 1:
            return "Connection in progress";
        case 2:
            return "Initial connection failed";
        case 3:
            return "Connection lost";
        case 4:
            return "Disconnection";
        case 5:
            return "Access Point";
        case 10:
            return "Connected";
        default:
            return "Unknown";
    }
}

String WiFiManager::getInfo(String name)
{
    if (name == "mac") {
        return WiFi.macAddress();
    }
    if (name == "ip") {
        return WiFi.localIP().toString();
    }
    if (name == "ssid") {
        return WiFi.SSID();
    }
    if (name == "rssi") {
        return String(WiFi.RSSI());
    }
    if (name == "status") {
        return String(WiFi.status());
    }
    if (name == "gateway") {
        return WiFi.gatewayIP().toString();
    }
    if (name == "subnet") {
        return WiFi.subnetMask().toString();
    }
    if (name == "dns") {
        return WiFi.dnsIP().toString();
    }
    if (name == "bssid") {
        return WiFi.BSSIDstr();
    }
    if (name == "channel") {
        return String(WiFi.channel());
    }
    if (name == "psk") {
        return WiFi.psk();
    }
    return "";
}

void WiFiManager::scanNetworks(bool show_hidden)
{
    WiFi.scanDelete();
    WiFi.scanNetworks(true, show_hidden);
}

int WiFiManager::getNetworks()
{
    return WiFi.scanComplete();
}

int WiFiManager::getNetworkCount(bool show_hidden)
{
    return WiFi.scanNetworks(false, show_hidden);
}

void WiFiManager::setNetwork(int n, bool save)
{
    this->ssid = WiFi.SSID(n);
    if (save) {
        this->saveSSID(this->ssid);
    }
}

String WiFiManager::getNetworkInfo(int n, String name)
{
    if (name == "ssid") {
        return WiFi.SSID(n);
    }
    if (name == "bssid") {
        return WiFi.BSSIDstr(n);
    }
    if (name == "channel") {
        return String(WiFi.channel(n));
    }
    if (name == "rssi") {
        return String(WiFi.RSSI(n));
    }
    if (name == "encryption") {
        return String(WiFi.encryptionType(n));
    }
    return "";
}

void WiFiManager::setPowerSave(bool value)
{
#ifdef ESP32
    WiFi.setSleep(value);
#endif
#ifdef ESP8266
    wifi_set_sleep_type(value ? LIGHT_SLEEP_T : NONE_SLEEP_T);
#endif
}

#ifndef DISABLE_ESPUI
void WiFiManager::initEspUI()
{
    logDebug("Init WiFi EspUI", 2);

    auto callback = std::bind(&WiFiManager::EspUiCallback, this, std::placeholders::_1, std::placeholders::_2);

    auto wifiTab = ESPUI.addControl(Tab, "", "WiFi");
    ssidInput = ESPUI.addControl(Text, "SSID", ssid, Peterriver, wifiTab, callback);
    passwordInput = ESPUI.addControl(Text, "Password", "", Peterriver, wifiTab, callback);

    ESPUI.setInputType(passwordInput, "password");
    ESPUI.addControl(Max, "", "32", None, ssidInput);
    ESPUI.addControl(Max, "", "64", None, passwordInput);

    auto wifisave = ESPUI.addControl(Button, "Save", "WiFiSave", Peterriver, wifiTab, callback);

    ESPUI.setEnabled(wifisave, true);

    auto connect = ESPUI.addControl(Button, "Connect", "WiFiConnect", Peterriver, wifiTab, callback);
    ESPUI.setEnabled(connect, true);
}

void WiFiManager::EspUiCallback(Control* sender, int type)
{
    logDebug(
        "WiFi ESPUI callback: sender.value = " + sender->value + " sender.id = " + sender->id + " sender.type = " + sender->type + "  / type = " + String(type),
        2);
    if (type == B_DOWN) {
        return;
    }

    if (sender->value == "WiFiConnect") {
        context->getEventManager()->triggerEvent("ESPUI", "WiFiConnect", {});
    } else if (sender->value == "WiFiSave") {
        std::vector<String> params1;
        params1.push_back(ESPUI.getControl(ssidInput)->value);
        context->getEventManager()->triggerEvent("ESPUI", "WiFiSaveSSID", params1);

        String password = ESPUI.getControl(passwordInput)->value;
        if (password.length() > 0) {
            std::vector<String> params2;
            params2.push_back(password);
            context->getEventManager()->triggerEvent("ESPUI", "WiFiSavePassword", params2);
        }
    }
}
#endif

#ifdef ESP8266
bool WiFiManager::otaUpdate()
{
    if (WiFi.status() != WL_CONNECTED) {
        logDebug("No WiFi connection", 1);
        return false;
    }

    if (connectionStatus != 10) {
        logDebug("No WiFi connection STA", 1);
        return false;
    }

    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    String otaHost = configMgr ? configMgr->getPreference("ota_host", configMgr->OTA_HOST) : "";
    int otaPort = configMgr ? configMgr->getPreference("ota_port", configMgr->OTA_PORT) : 443;
    if (otaHost.length() == 0) {
        logDebug("No OTA Host", 1);
        return false;
    }

    String otaFingerprint = configMgr ? configMgr->OTA_FINGERPRINT : "";

    String otaUrl = configMgr ? configMgr->getPreference("ota_url", configMgr->OTA_URL) : "";
    if (otaUrl.length() == 0) {
        logDebug("No OTA URL", 1);
        return false;
    }

    //WiFiClient client;

    WiFiClientSecure client;
    bool mfln = client.probeMaxFragmentLength(otaHost, otaPort, 1024);
    if (mfln) {
        logDebug("Maximum fragment Length negotiation supported.", 2);
        client.setBufferSizes(1024, 1024);
    }
    client.setInsecure();

    if (!client.connect(otaHost, otaPort)) {
        logDebug("Connection to " + otaHost + ":" + String(otaPort) + " failed", 1);
        return false;
    } else {
        logDebug("Connected to " + otaHost + ":" + String(otaPort), 2);
    }

    logDebug("Start OTA update from " + otaHost + ":" + String(otaPort) + otaUrl, 1);
    auto ret = ESPhttpUpdate.update(client, otaHost, otaPort, otaUrl);
    // if successful, ESP will restart
    switch (ret) {
        case HTTP_UPDATE_FAILED:
            logDebug("OTA Update failed: " + ESPhttpUpdate.getLastErrorString(), 1);
            return false;
        case HTTP_UPDATE_NO_UPDATES:
            logDebug("OTA No updates", 1);
            return false;
        case HTTP_UPDATE_OK:
            logDebug("OTA Update successful", 1);  // may not be called since we reboot the ESP
            return true;
    }
    return false;
}
#endif
#ifdef ESP32

// Non testé
bool WiFiManager::otaUpdate()
{
    if (WiFi.status() != WL_CONNECTED) {
        logDebug("No WiFi connection", 1);
        return false;
    }

    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    String otaHost = configMgr ? configMgr->getPreference("ota_host", configMgr->OTA_HOST) : "";
    int otaPort = configMgr ? configMgr->getPreference("ota_port", configMgr->OTA_PORT) : 443;
    String otaUrl = configMgr ? configMgr->getPreference("ota_url", configMgr->OTA_URL) : "";

    if (otaHost.length() == 0) {
        logDebug("No OTA Host", 1);
        return false;
    }

    if (otaUrl.length() == 0) {
        logDebug("No OTA URL", 1);
        return false;
    }

    // Construire l'URL complète
    String fullUrl = "https://" + otaHost + ":" + String(otaPort) + otaUrl;

    // Configuration pour la mise à jour OTA
    esp_http_client_config_t ota_config = {
        .url = fullUrl.c_str(),
        .cert_pem = configMgr ? configMgr->OTA_CERT_PEM : nullptr,  //NULL,  // Utilisez un certificat si nécessaire pour la sécurité
        //.skip_cert_common_name_check = true,
    };

    logDebug("Starting OTA update from " + fullUrl, 1);

    // Démarrage de la mise à jour OTA
    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        logDebug("OTA Update successful", 1);
        esp_restart();
        return true;
    } else {
        logDebug("OTA Update failed: " + String(esp_err_to_name(ret)), 1);
        return false;
    }
}
#endif

// Saved Networks Management Implementation
bool WiFiManager::saveNetwork(const String& ssid, const String& password, int priority)
{
    if (ssid.isEmpty())
        return false;

    // If priority is 0 (default), assign highest priority automatically
    if (priority == 0) {
        int maxPriority = 0;
        for (const auto& network : savedNetworks) {
            if (network.priority > maxPriority) {
                maxPriority = network.priority;
            }
        }
        priority = maxPriority + 10;  // Give new network highest priority
        logDebug("Auto-assigned priority " + String(priority) + " to new network: " + ssid, 2);
    }

    // Check if network already exists and update it
    for (auto& network : savedNetworks) {
        if (network.ssid == ssid) {
            network.password = password;
            // Update priority if it's different or if auto-assigning (priority == 0)
            if (priority == 0 || priority != network.priority) {
                // If auto-assigning for existing network, keep current priority
                if (priority != 0) {
                    network.priority = priority;
                }
            }
            saveSavedNetworksToPrefs();
            logDebug("Updated saved network: " + ssid + " with priority " + String(network.priority), 2);
            return true;
        }
    }

    // Add new network
    SavedNetwork newNetwork;
    newNetwork.ssid = ssid;
    newNetwork.password = password;
    newNetwork.priority = priority;

    savedNetworks.push_back(newNetwork);
    saveSavedNetworksToPrefs();
    logDebug("Added new saved network: " + ssid + " with priority " + String(priority), 2);
    return true;
}

bool WiFiManager::removeNetwork(const String& ssid)
{
    for (auto it = savedNetworks.begin(); it != savedNetworks.end(); ++it) {
        if (it->ssid == ssid) {
            savedNetworks.erase(it);
            saveSavedNetworksToPrefs();
            logDebug("Removed saved network: " + ssid, 2);
            return true;
        }
    }
    return false;
}

void WiFiManager::listSavedNetworks()
{
    if (savedNetworks.empty()) {
        logDebug("No saved networks", 0);
        return;
    }

    logDebug("Saved networks:", 0);
    for (size_t i = 0; i < savedNetworks.size(); i++) {
        String info = String(i) + ": " + savedNetworks[i].ssid;
        if (savedNetworks[i].priority > 0) {
            info += " (priority: " + String(savedNetworks[i].priority) + ")";
        }
        logDebug(info, 0);
    }
}

void WiFiManager::clearSavedNetworks()
{
    savedNetworks.clear();

    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    if (configMgr) {
        configMgr->setPreference("saved_networks", "");
    }

    logDebug("Cleared all saved networks", 2);
}

void WiFiManager::loadSavedNetworks()
{
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    if (!configMgr)
        return;

    String networksJson = configMgr->getPreference("saved_networks", "[]");

    // If no saved networks, try to migrate from legacy system
    if (networksJson == "[]") {
        String legacySSID = configMgr->getPreference("wf_ssid", "");
        String legacyPass = configMgr->getPreference("wf_pass", "");

        if (!legacySSID.isEmpty() && !legacyPass.isEmpty()) {
            logDebug("Migrating legacy network to unified system: " + legacySSID, 1);
            // Create the legacy network with priority 1000 (highest)
            if (saveNetwork(legacySSID, legacyPass, 1000)) {
                logDebug("Legacy network migrated successfully", 1);
                // Don't delete legacy preferences yet - keep for compatibility
            }
            return;  // We've migrated, savedNetworks is now populated
        } else {
            logDebug("No saved networks or legacy networks found", 2);
            return;
        }
    }

    // Parse JSON and load networks
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, networksJson);

    if (error) {
        logDebug("Failed to parse saved networks JSON: " + String(error.c_str()), 1);
        return;
    }

    savedNetworks.clear();
    JsonArray networks = doc.as<JsonArray>();

    for (JsonObject network : networks) {
        SavedNetwork saved;
        saved.ssid = network["ssid"].as<String>();
        saved.password = network["password"].as<String>();
        saved.priority = network["priority"].as<int>();

        if (!saved.ssid.isEmpty()) {
            savedNetworks.push_back(saved);
        }
    }

    logDebug("Loaded " + String(savedNetworks.size()) + " saved networks", 2);
}

void WiFiManager::saveSavedNetworksToPrefs()
{
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    if (!configMgr)
        return;

    JsonDocument doc;
    JsonArray networks = doc.to<JsonArray>();

    for (const auto& network : savedNetworks) {
        JsonObject netObj = networks.add<JsonObject>();
        netObj["ssid"] = network.ssid;
        netObj["password"] = network.password;
        netObj["priority"] = network.priority;
    }

    String json;
    serializeJson(doc, json);
    configMgr->setPreference("saved_networks", json);

    logDebug("Saved networks to preferences", 3);
}

bool WiFiManager::connectToSavedNetwork()
{
    if (savedNetworks.empty()) {
        logDebug("No saved networks available", 2);
        return false;
    }

    // Sort networks by priority (highest first)
    std::sort(savedNetworks.begin(), savedNetworks.end(), [](const SavedNetwork& a, const SavedNetwork& b) { return a.priority > b.priority; });

    // Try each network in priority order
    for (const auto& network : savedNetworks) {
        logDebug("Trying saved network: " + network.ssid, 1);
        this->ssid = network.ssid;
        this->password = network.password;

        if (connect()) {
            logDebug("Connected to saved network: " + network.ssid, 1);
            return true;
        }

        // Wait a bit before trying next network
        delay(NETWORK_RETRY_DELAY_MS);
    }

    logDebug("Failed to connect to any saved network", 1);
    return false;
}

// Interactive Password Prompt Implementation
void WiFiManager::registerCommands()
{
    auto* cmdMgr = static_cast<CommandManager*>(context ? context->getManager("CommandManager") : nullptr);
    if (!cmdMgr)
        return;

    // WiFi connection commands
    cmdMgr->registerCommand(Command("wifi", "connect", "Connect to WiFi network", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
        connect();
        return "WiFi connection initiated";
    }));

    cmdMgr->registerCommand(Command("wifi", "disconnect", "Disconnect from WiFi", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
        disconnect();
        return "WiFi disconnected";
    }));

    cmdMgr->registerCommand(Command("wifi", "ap", "Start WiFi Access Point mode", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
        startAccessPoint();
        return "WiFi Access Point started";
    }));

    cmdMgr->registerCommand(Command("wifi", "ssid", "Get/Set primary WiFi SSID", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
        if (args.size() > 0) {
            String newSSID = args[0];

            // Check if network already exists in saved networks
            for (const auto& network : savedNetworks) {
                if (network.ssid == newSSID) {
                    // Network already saved - just update current and connect
                    this->ssid = newSSID;
                    this->password = network.password;
                    return "Using saved network: " + newSSID + "\nConnecting...";
                }
            }

            // New network - prompt for password
            auto* cmdMgr = static_cast<CommandManager*>(context->getManager("CommandManager"));
            if (cmdMgr) {
                String prompt = "Enter password for network: " + newSSID + ":";
                cmdMgr->requestInput(prompt, [this, newSSID](const String& password) {
                    if (password.isEmpty()) {
                        // Cancelled
                        debug("Password input cancelled", 1);
                        return;
                    }
                    
                    // Save and connect
                    if (saveNetwork(newSSID, password, 0)) {
                        this->ssid = newSSID;
                        this->password = password;
                        connect();
                    }
                });
                return "";  // Return empty, prompt handles the message
            }
            return "CommandManager not available";
        }

        // Show current SSID (connected or configured)
        if (WiFi.status() == WL_CONNECTED) {
            return "Connected SSID: " + WiFi.SSID();
        } else if (!savedNetworks.empty()) {
            // Find highest priority network
            auto maxNetwork = std::max_element(savedNetworks.begin(), savedNetworks.end(),
                                               [](const SavedNetwork& a, const SavedNetwork& b) { return a.priority < b.priority; });
            return "Primary SSID: " + maxNetwork->ssid;
        } else {
            return "SSID: " + (ssid.isEmpty() ? "(none)" : ssid);
        }
    }));

    cmdMgr->registerCommand(
        Command("wifi", "pass", "Set WiFi password for primary SSID", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                if (ssid.isEmpty()) {
                    return "No SSID set. Use wifi:ssid <ssid> first";
                }
                // Save the network with highest priority (1000) to make it primary
                if (saveNetwork(ssid, args[0], 1000)) {
                    this->password = args[0];  // Update legacy password
                    return "Password saved for primary network: " + ssid;
                } else {
                    return "Failed to save network";
                }
            }
            return "Use: wifi:pass <password> to set password for SSID: " + (ssid.isEmpty() ? "(none)" : ssid);
        }));

    cmdMgr->registerCommand(Command("wifi", "reset", "Reset WiFi credentials", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
        clearSavedNetworks();
        saveSSID("");
        savePassword("");
        return "All WiFi credentials cleared";
    }));

    cmdMgr->registerCommand(
        Command("wifi", "autoconnect", "Auto-connect to saved WiFi", CommandSource::Any, true,
                [this](const std::vector<String>& args) -> String { return autoConnect() ? "WiFi auto-connect successful" : "WiFi auto-connect failed"; }));

    cmdMgr->registerCommand(
        Command("wifi", "status", "Show WiFi connection status", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
            String result = "WiFi Status:\n";
            result += "  Connected: " + String(isConnected() ? "Yes" : "No") + "\n";
            result += "  Status: " + getStatus() + "\n";
            result += "  SSID: " + retrieveSSID() + "\n";
            if (isConnected()) {
                result += "  IP: " + retrieveIP() + "\n";
                result += "  Signal: " + String(WiFi.RSSI()) + " dBm";
            }
            return result;
        }));

    cmdMgr->registerCommand(
        Command("wifi", "scan", "Scan for available WiFi networks", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
            // Start scan
            WiFi.scanDelete();        // Clear previous results
            WiFi.scanNetworks(true);  // Start async scan

            // Wait for scan to complete (with timeout)
            int timeout = CONNECTION_TIMEOUT_MS;  // 10 seconds
            int startTime = millis();
            int count = -1;

            while (millis() - startTime < timeout) {
                count = WiFi.scanComplete();
                if (count >= 0)
                    break;  // Scan complete
                delay(100);
            }

            if (count == -1) {
                return "WiFi scan timeout. Try again.";
            } else if (count == 0) {
                return "No networks found.";
            }

            String result = "Found " + String(count) + " networks:\n";
            for (int i = 0; i < count; i++) {
                result += "  " + String(i) + ": " + getNetworkInfo(i, "ssid");
                result += " (" + getNetworkInfo(i, "rssi") + " dBm)";
                if (getNetworkInfo(i, "encryption") != "Open") {
                    result += " [Protected]";
                }
                result += "\n";
            }
            result += "\nUse wifi:network <n> to select a network";
            return result;
        }));

    cmdMgr->registerCommand(
        Command("wifi", "network", "Select network from scan by number", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                int networkIndex = args[0].toInt();
                int count = WiFi.scanComplete();

                if (count == -1) {
                    return "No scan results available. Run wifi:scan first";
                }

                if (networkIndex < 0 || networkIndex >= count) {
                    return "Invalid network number. Use 0-" + String(count - 1);
                }

                String selectedSSID = getNetworkInfo(networkIndex, "ssid");
                this->ssid = selectedSSID;  // Set for legacy compatibility

                // Check if network already exists in saved networks
                for (const auto& network : savedNetworks) {
                    if (network.ssid == selectedSSID) {
                        // Network already saved - just use it
                        this->password = network.password;
                        return "Using saved network: " + selectedSSID + "\nUse wifi:connect to connect";
                    }
                }

                // Check if network is open (no password needed)
                String encryption = getNetworkInfo(networkIndex, "encryption");
                if (encryption == "Open" || encryption == "0") {
                    // Open network - save without password
                    if (saveNetwork(selectedSSID, "", 1000)) {
                        this->password = "";
                        debug("Connecting to open network: " + selectedSSID, 1);
                        if (connect()) {
                            return "Connected to open network: " + selectedSSID;
                        } else {
                            return "Failed to connect to: " + selectedSSID;
                        }
                    } else {
                        return "Failed to save network: " + selectedSSID;
                    }
                }

                // Protected network - prompt for password
                auto* cmdMgr = static_cast<CommandManager*>(context->getManager("CommandManager"));
                if (cmdMgr) {
                    String prompt = "Enter password for network: " + selectedSSID + ":";
                    cmdMgr->requestInput(prompt, [this, selectedSSID](const String& password) {
                        if (password.isEmpty()) {
                            // Cancelled
                            debug("Password input cancelled", 1);
                            return;
                        }
                        
                        // Save and connect
                        if (saveNetwork(selectedSSID, password, 0)) {
                            this->ssid = selectedSSID;
                            this->password = password;
                            connect();
                        }
                    });
                    return "";  // Return empty, prompt handles the message
                }
                return "CommandManager not available";
            } else {
                return "Current network: " + (ssid.isEmpty() ? "(none)" : ssid) + "\nUsage: wifi:network <number>";
            }
        }));

    cmdMgr->registerCommand(
        Command("wifi", "info", "Show detailed WiFi information", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
            String result = "WiFi Information:\n";
            result += "  SSID: " + retrieveSSID() + "\n";
            result += "  Connected: " + String(isConnected() ? "Yes" : "No") + "\n";
            if (isConnected()) {
                result += "  IP Address: " + retrieveIP() + "\n";
                result += "  Signal Strength: " + String(WiFi.RSSI()) + " dBm\n";
                result += "  MAC Address: " + WiFi.macAddress() + "\n";
                result += "  Gateway: " + WiFi.gatewayIP().toString() + "\n";
                result += "  DNS: " + WiFi.dnsIP().toString();
            } else {
                result += "  Status: " + getStatus();
            }
            return result;
        }));

    // Saved networks management commands
    cmdMgr->registerCommand(
        Command("wifi", "save", "Save WiFi network credentials", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
            if (args.size() >= 2) {
                int priority = args.size() > 2 ? args[2].toInt() : 0;
                if (saveNetwork(args[0], args[1], priority)) {
                    return "Network saved: " + args[0];
                } else {
                    return "Failed to save network";
                }
            }
            return "Usage: wifi:save <ssid> <password> [priority]";
        }));

    cmdMgr->registerCommand(Command("wifi", "remove", "Remove saved WiFi network", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
        if (args.size() > 0) {
            if (removeNetwork(args[0])) {
                return "Network removed: " + args[0];
            } else {
                return "Network not found: " + args[0];
            }
        }
        return "Usage: wifi:remove <ssid>";
    }));

    cmdMgr->registerCommand(Command("wifi", "list", "List saved WiFi networks", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
        if (savedNetworks.empty()) {
            return "No saved networks";
        }

        String result = "Saved networks:\n";
        for (size_t i = 0; i < savedNetworks.size(); i++) {
            result += "  " + String(i) + ": " + savedNetworks[i].ssid;
            if (savedNetworks[i].priority > 0) {
                result += " (priority: " + String(savedNetworks[i].priority) + ")";
            }
            result += "\n";
        }
        return result;
    }));

    cmdMgr->registerCommand(Command("wifi", "clear", "Clear all saved networks", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
        clearSavedNetworks();
        return "All saved networks cleared";
    }));

    cmdMgr->registerCommand(
        Command("wifi", "tryall", "Try to connect to saved networks", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
            if (connectToSavedNetwork()) {
                return "Connected to saved network: " + ssid;
            } else {
                return "Failed to connect to any saved network";
            }
        }));

    // Legacy commands not migrated yet
    cmdMgr->registerCommand(
        Command("wifi", "debug", "Show WiFi debug information", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
            return "SSID: " + retrieveSSID() + "\nPassword: " + (retrievePassword().isEmpty() ? "(none)" : "***");
        }));

    cmdMgr->registerCommand(Command("wifi", "keep", "Toggle keep connection mode", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
        bool isKeepOn = keepConnection();
        return "Keep connection: " + String(isKeepOn ? "ON" : "OFF");
    }));

    cmdMgr->registerCommand(Command("wifi", "telnet", "Setup Telnet server", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
        setupTelnet();
        return "Telnet server started";
    }));

    cmdMgr->registerCommand(Command("wifi", "cancel", "Cancel password prompt", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
        auto* cmdMgr = static_cast<CommandManager*>(context->getManager("CommandManager"));
        if (cmdMgr && cmdMgr->isWaitingForInput()) {
            cmdMgr->cancelInput();
            return "Input cancelled";
        }
        return "No active input request";
    }));

    cmdMgr->registerCommand(
        Command("wifi", "priority", "Set priority for a saved network", CommandSource::Any, true, [this](const std::vector<String>& args) -> String {
            if (args.size() < 2) {
                return "Usage: wifi:priority <ssid> <priority>\nHigher priority = preferred network";
            }

            String targetSSID = args[0];
            int newPriority = args[1].toInt();

            for (auto& network : savedNetworks) {
                if (network.ssid == targetSSID) {
                    network.priority = newPriority;
                    saveSavedNetworksToPrefs();
                    return "Updated priority for " + targetSSID + " to " + String(newPriority);
                }
            }

            return "Network not found: " + targetSSID;
        }));

    // Register useful aliases
    cmdMgr->registerAlias("ws", "wifi:status");
    cmdMgr->registerAlias("wi", "wifi:info");
    cmdMgr->registerAlias("wc", "wifi:connect");
    cmdMgr->registerAlias("wscan", "wifi:scan");
    cmdMgr->registerAlias("wlist", "wifi:list");
    cmdMgr->registerAlias("wsave", "wifi:save");
}
