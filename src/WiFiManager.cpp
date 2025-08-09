#include <WiFiManager.h>
#include <ConfigurationManager.h>
#include <EventManager.h>
#include <TimeManager.h>
#include <MQTTManager.h>
#include <CommandManager.h>


void WiFiManager::init()
{
    init(true);
}

void WiFiManager::init(bool auto_connect)
{
    logDebug("Init WiFiManager", 1);
    retrieveSSID();
    retrievePassword();
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    apMode = static_cast<wm_ap_mode>(configMgr ? configMgr->getPreference("ap_mode", 2) : 2);

    if (!this->ssid.length() || this->ssid == "") {
        logDebug("No SSID, Start Access Point", 0);
        startAccessPoint();
    } else if (auto_connect) {
        this->autoConnect();
    }
    
    // Register WiFi commands with CommandManager
    registerCommands();
    
    setInitialized(true);
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
            //Serial.println(WiFi.status());
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
                if (tryCount >= 20) {
                    tryCount = 0;
                    if (keepConnected) {
                        logDebug("WiFi: Connection failed, retrying", 1);
                    } else {
                        if (connectionStatus != 6) {
                            connectionStatus = 2;
                            logDebug("WiFi: Connection failed", 1);
                            context->getEventManager()->triggerEvent("wifi", "failed", params);
                            // if wifi mode is AP, restart AP
                            this->startAccessPoint();
                        }
                    }
                } else {
                    //Serial.print("*");
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
            return true;
            
        } else if (event == "ap_started") {
            debug("WiFi Access Point started", 1);
            // ESPUI.begin(); // Could be handled here if needed
            return true;
            
        } else if (event == "disconnected" || event == "lost") {
            // Update MQTT status when WiFi disconnects
            auto* mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
            if (mqttMgr) {
                mqttMgr->setStatus(1);
            }
            
            debug("WiFi disconnected", 1);
            return true;
            
        } else if (event.startsWith("@")) {
            // Handle WiFi commands via events
            return onCommand(event.substring(1), params);
        }
    }
    
    return false; // Event not handled
}

bool WiFiManager::onCommand(const String& command, const std::vector<String>& params)
{
    logDebug("Processing WiFi command: " + command, 3);
    if (command == "connect") {
        this->connect();
    } else if (command == "disconnect") {
        this->disconnect();
    } else if (command == "ap") {
        this->startAccessPoint();
    } else if (command == "ssid") {
        if (params.size() > 0) {
            this->saveSSID(params[0]);
        } else {
            logDebug("SSID: " + retrieveSSID(), 0);
        }
    } else if (command == "pass") {
        if (params.size() > 0) {
            this->savePassword(params[0]);
        } else {
            logDebug("Password: " + retrievePassword(), 0);
        }
    } else if (command == "reset") {
        this->saveSSID("");
        this->savePassword("");
    } else if (command == "autoconnect") {
        this->autoConnect();
    } else if (command == "status" || command == "") {
        logDebug("Connected: " + String(isConnected()), 0);
        logDebug("Status: " + getStatus(), 0);
        logDebug("SSID: " + getInfo("ssid"), 0);
        logDebug("IP: " + getInfo("ip"), 0);
        logDebug("MAC: " + getInfo("mac"), 0);
        logDebug("RSSI: " + getInfo("rssi"), 0);
    } else if (command == "debug") {
        logDebug("Debug infos:", 0);
    } else if (command == "keep") {
        if (keepConnection()) {
            logDebug("Keep connection: ON", 0);
        } else {
            logDebug("Keep connection: OFF", 0);
        }
    } else if (command == "scan") {
        uint16_t count = getNetworkCount();
        logDebug("Networks found: " + String(count), 0);
        for (int i = 0; i < count; i++) {
            logDebug("#" + String(i) + " " + getNetworkInfo(i, "ssid") + " RSSI=" + getNetworkInfo(i, "rssi") + "db", 0);
        }
        logDebug("wifi:network <n> to set network", 0);
    } else if (command == "network") {
        if (params.size() > 0) {
            this->setNetwork(params[0].toInt(), true);
            logDebug("Network set to: " + getSSID(), 0);
        } else {
            logDebug("Current network: " + getSSID(), 0);
        }
    } else if (command == "telnet") {
        setupTelnet();
#ifdef ESP32
    } else if (command == "ping") {
        if (params.size() > 0) {
            logDebug("Ping: " + params[0], 0);
            // NOTE: Ping functionality currently disabled due to library dependency
            logDebug("Ping functionality not available", 1);
            /*
            IPAddress ip;
            if (ip.fromString(params[0])) {
                if (Ping.ping(ip, 1)) {
                    eventManager->debug("Ping successful", 0);
                } else {
                    eventManager->debug("Ping failed", 0);
                }
            } else {
                logDebug("Invalid IP address", 1);
            }
            */
        } else {
            logDebug("Missing IP address", 1);
        }
#endif
    } else {
        return false;
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
    delay(100);

    if (WiFi.status() == WL_CONNECTED) {
        setConnected();
        return true;
    } else {
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
    return this->ssid;
}

bool WiFiManager::isConnected()
{
    return this->connected;
}

void WiFiManager::saveSSID(String ssid, bool reconnect)
{
    this->ssid = ssid;
    logDebug("New WiFi SSID: " + this->ssid, 1);
    
    // Use ConfigurationManager instead of old Configuration
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    if (configMgr) {
        configMgr->setPreference("wf_ssid", ssid);
        logDebug("SSID saved to preferences", 2);
    } else {
        logDebug("ConfigurationManager not available for saving SSID", 1);
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
    
    // Use ConfigurationManager instead of old Configuration
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    if (configMgr) {
        configMgr->setPreference("wf_pass", password);
        logDebug("Password saved to preferences", 2);
    } else {
        logDebug("ConfigurationManager not available for saving password", 1);
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

    String otaFingerprint = config ? config->OTA_FINGERPRINT : "";

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

void WiFiManager::registerCommands()
{
    auto* cmdMgr = static_cast<CommandManager*>(context ? context->getManager("CommandManager") : nullptr);
    if (!cmdMgr) return;

    // WiFi connection commands
    cmdMgr->registerCommand(Command(
        "wifi", "connect", "Connect to WiFi network",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            connect();
            return "WiFi connection initiated";
        }
    ));

    cmdMgr->registerCommand(Command(
        "wifi", "disconnect", "Disconnect from WiFi",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            disconnect();
            return "WiFi disconnected";
        }
    ));

    cmdMgr->registerCommand(Command(
        "wifi", "ap", "Start WiFi Access Point mode",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            startAccessPoint();
            return "WiFi Access Point started";
        }
    ));

    cmdMgr->registerCommand(Command(
        "wifi", "ssid", "Get/Set WiFi SSID",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                saveSSID(args[0]);
                return "SSID set to: " + args[0];
            }
            return "SSID: " + retrieveSSID();
        }
    ));

    cmdMgr->registerCommand(Command(
        "wifi", "pass", "Get/Set WiFi password",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                savePassword(args[0]);
                return "WiFi password updated";
            }
            return "Password: " + String(retrievePassword().isEmpty() ? "(none)" : "***");
        }
    ));

    cmdMgr->registerCommand(Command(
        "wifi", "reset", "Reset WiFi credentials",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            saveSSID("");
            savePassword("");
            return "WiFi credentials cleared";
        }
    ));

    cmdMgr->registerCommand(Command(
        "wifi", "autoconnect", "Auto-connect to saved WiFi",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            return autoConnect() ? "WiFi auto-connect successful" : "WiFi auto-connect failed";
        }
    ));

    cmdMgr->registerCommand(Command(
        "wifi", "status", "Show WiFi connection status",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            String result = "WiFi Status:\n";
            result += "  Connected: " + String(isConnected() ? "Yes" : "No") + "\n";
            result += "  Status: " + getStatus() + "\n";
            result += "  SSID: " + retrieveSSID() + "\n";
            if (isConnected()) {
                result += "  IP: " + retrieveIP() + "\n";
                result += "  Signal: " + String(WiFi.RSSI()) + " dBm";
            }
            return result;
        }
    ));

    cmdMgr->registerCommand(Command(
        "wifi", "scan", "Scan for available WiFi networks",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            scanNetworks();
            int count = getNetworkCount();
            String result = "Found " + String(count) + " networks:\n";
            for (int i = 0; i < count; i++) {
                result += "  " + String(i) + ": " + getNetworkInfo(i, "ssid");
                result += " (" + getNetworkInfo(i, "rssi") + " dBm)";
                if (getNetworkInfo(i, "encryption") != "Open") {
                    result += " [Protected]";
                }
                result += "\n";
            }
            return result;
        }
    ));

    cmdMgr->registerCommand(Command(
        "wifi", "info", "Show detailed WiFi information",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
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
        }
    ));


    // Register useful aliases
    cmdMgr->registerAlias("ws", "wifi:status");
    cmdMgr->registerAlias("wi", "wifi:info");
    cmdMgr->registerAlias("wc", "wifi:connect");
    cmdMgr->registerAlias("wscan", "wifi:scan");
}
