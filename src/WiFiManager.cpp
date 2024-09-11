#include "../include/WiFiManager.h"

EventManager* WiFiManager::eventManager = nullptr;

void WiFiManager::init(bool auto_connect)
{
    eventManager->debug("Init WiFiManager", 1);
    retrieveSSID();
    retrievePassword();
    if (auto_connect) {
        this->autoConnect();
    }
}

void WiFiManager::loop()
{
    static unsigned long lastMillis = 0;
    unsigned long currentMillis = millis();

    if (connectionStatus == 0) {
        this->connect();
        lastMillis = currentMillis;
        connectionStatus = 1;
    }

    unsigned long wait = (tryCount + 1) * checkDelay;

    if (currentMillis - lastMillis >= wait) {
        if (connectionStatus == 1)  // Connection in progress
        {
            if (WiFi.status() != WL_CONNECTED) {
                this->connected = false;
                tryCount++;
                std::vector<String> params;
                params.push_back(String(tryCount));
                eventManager->triggerEvent("wifi", "in_progress", params);
                if (tryCount >= 20) {
                    tryCount = 0;
                    if (keepConnected) {
                        eventManager->debug("WiFi: Connection failed, retrying", 1);
                    } else {
                        connectionStatus = 2;
                        eventManager->debug("WiFi: Connection failed", 1);
                        eventManager->triggerEvent("wifi", "failed", params);
                        this->startAccessPoint();
                    }
                } else {
                    //Serial.print("*");
                    eventManager->debug("WiFi: connection in progress #" + String(tryCount) + "..." , 2);
                }
            } else {
                connected = true;
                keepConnected = true;
                connectionStatus = 10;
                tryCount = 0;
                std::vector<String> params;
                params.push_back(WiFi.SSID());
                params.push_back(WiFi.localIP().toString());
                eventManager->triggerEvent("wifi", "connected", params);
            }
        } else if (connectionStatus == 10)  // Connected
        {
            if (WiFi.status() != WL_CONNECTED) {
                connectionStatus = 3;
                this->connected = false;
                eventManager->debug("WiFi: Connection lost", 1);
                eventManager->triggerEvent("wifi", "lost", {});
            }
        } else if (connectionStatus == 3)  // Connection lost
        {
            if (WiFi.status() == WL_CONNECTED) {
                connectionStatus = 10;
                this->connected = true;
                eventManager->triggerEvent("wifi", "recovered", {});
            }
        }
        lastMillis = currentMillis;
    }
}

void WiFiManager::processEvent(String type, String event, std::vector<String> params)
{
    if (type == "wifi") {
        if (event.startsWith("@")) {
            processCommand(event.substring(1), params);
        } else {
            processCommand(event, params);
        }
    }
}

bool WiFiManager::processCommand(String command, std::vector<String> params)
{
    eventManager->debug("Processing WiFi command: " + command, 3);
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
            eventManager->debug("SSID: " + retrieveSSID(), 0);
        }
    } else if (command == "pass") {
        if (params.size() > 0) {
            this->savePassword(params[0]);
        } else {
            eventManager->debug("Password: " + retrievePassword(), 0);
        }
    } else if (command == "reset") {
        this->saveSSID("");
        this->savePassword("");
    } else if (command == "autoconnect") {
        this->autoConnect();
    } else if (command == "status" || command == "") {
        eventManager->debug("Connected: " + String(isConnected()), 0);
        eventManager->debug("Status: " + getStatus(), 0);
        eventManager->debug("SSID: " + getInfo("ssid"), 0);
        eventManager->debug("IP: " + getInfo("ip"), 0);
        eventManager->debug("MAC: " + getInfo("mac"), 0);
        eventManager->debug("RSSI: " + getInfo("rssi"), 0);
    } else if (command == "debug") {
        eventManager->debug("Debug infos:", 0);
    } else if (command == "keep") {
        if (keepConnection()) {
            eventManager->debug("Keep connection: ON", 0);
        } else {
            eventManager->debug("Keep connection: OFF", 0);
        }
    } else if (command == "scan") {
        uint16_t count = getNetworkCount();
        eventManager->debug("Networks found: " + String(count), 0);
        for (int i = 0; i < count; i++) {
            eventManager->debug(String(i) + ": " + getNetworkInfo(i, "ssid"), 0);
        }
        eventManager->debug("wifi:network <n> to set network", 0);
    } else if (command == "network") {
        if (params.size() > 0) {
            this->setNetwork(params[0].toInt(), true);
            eventManager->debug("Network set to: " + getSSID(), 0);
        } else {
            eventManager->debug("Current network: " + getSSID(), 0);
        }
    } else {
        return false;
    }
    return true;
}

bool WiFiManager::autoConnect()
{
    if (WiFi.status() != WL_CONNECTED) {
        eventManager->debug("WiFi AutoConnect...", 0);
        if (this->ssid == "") {
            eventManager->debug("No SSID, starting access point", 0);
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
    eventManager->debug("Connecting to: " + this->ssid + "...", 0);
    WiFi.mode(WIFI_STA);

    WiFi.begin(this->ssid.c_str(), this->password.c_str());
    delay(100);

    if (WiFi.status() == WL_CONNECTED) {
        this->connected = true;
        connectionStatus = 10;
        return true;
    } else {
        return false;
    }
}

void WiFiManager::disconnect()
{
    WiFi.disconnect();
    this->connected = false;
    connectionStatus = 4;
    eventManager->triggerEvent("wifi", "disconnected", {});
}

void WiFiManager::startAccessPoint()
{
    disconnect();
    eventManager->debug("Creating Hotspot: " + config.getHostname(), 0);
    eventManager->debug("IP Address: " + this->apIP.toString(), 0);
    WiFi.mode(WIFI_AP);
    delay(100);
    WiFi.softAPConfig(this->apIP, this->apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(this->config.getHostname().c_str());
    connectionStatus = 5;
    connected = false;
    eventManager->triggerEvent("wifi", "ap_started", {});
}

String WiFiManager::getSSID()
{
    return this->ssid;
}

bool WiFiManager::isConnected()
{
    return this->connected;
}

void WiFiManager::saveSSID(String ssid)
{
    this->ssid = ssid;
    eventManager->debug("New WiFi SSID: " + this->ssid, 1);
    config.setPreference("wf_ssid", ssid);
}

void WiFiManager::savePassword(String password)
{
    this->password = password;
    eventManager->debug("New WiFi password: " + this->password, 1);
    config.setPreference("wf_pass", password);
}

String WiFiManager::getDebugInfos()
{
    return "SSID: " + retrieveSSID() + "\nPassword: " + retrievePassword();
}

String WiFiManager::retrieveSSID()
{
    ssid = config.getPreference("wf_ssid", ssid);
    return ssid;
}

String WiFiManager::retrievePassword()
{
    password = config.getPreference("wf_pass", password);
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

#ifndef DISABLE_ESPUI
void WiFiManager::initEspUI()
{
    eventManager->debug("Init WiFi EspUI", 2);

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
    eventManager->debug(
        "WiFi ESPUI callback: sender.value = " + sender->value + " sender.id = " + sender->id + " sender.type = " + sender->type + "  / type = " + String(type),
        2);
    if (type == B_DOWN) {
        return;
    }

    if (sender->value == "WiFiConnect") {
        eventManager->triggerEvent("ESPUI", "WiFiConnect", {});
    } else if (sender->value == "WiFiSave") {
        std::vector<String> params1;
        params1.push_back(ESPUI.getControl(ssidInput)->value);
        eventManager->triggerEvent("ESPUI", "WiFiSaveSSID", params1);

        String password = ESPUI.getControl(passwordInput)->value;
        if (password.length() > 0) {
            std::vector<String> params2;
            params2.push_back(password);
            eventManager->triggerEvent("ESPUI", "WiFiSavePassword", params2);
        }
    }
}
#endif

#ifdef ESP8266
bool WiFiManager::otaUpdate()
{
    if (WiFi.status() != WL_CONNECTED) {
        eventManager->debug("No WiFi connection", 1);
        return false;
    }

    if (connectionStatus != 10) {
        eventManager->debug("No WiFi connection STA", 1);
        return false;
    }

    String otaHost = config.getPreference("ota_host", config.OTA_HOST);
    int otaPort = config.getPreference("ota_port", config.OTA_PORT);
    if (otaHost.length() == 0) {
        eventManager->debug("No OTA Host", 1);
        return false;
    }

    String otaFingerprint = config.OTA_FINGERPRINT;

    String otaUrl = config.getPreference("ota_url", config.OTA_URL);
    if (otaUrl.length() == 0) {
        eventManager->debug("No OTA URL", 1);
        return false;
    }

    //WiFiClient client;

    WiFiClientSecure client;
    bool mfln = client.probeMaxFragmentLength(otaHost, otaPort, 1024);
    if (mfln) {
        eventManager->debug("Maximum fragment Length negotiation supported.", 2);
        client.setBufferSizes(1024, 1024);
    }
    client.setInsecure();

    if (!client.connect(otaHost, otaPort)) {
        eventManager->debug("Connection to " + otaHost + ":" + String(otaPort) + " failed", 1);
        return false;
    } else {
        eventManager->debug("Connected to " + otaHost + ":" + String(otaPort), 2);
    }

    /*if (!client.verify(otaFingerprint.c_str(), otaHost.c_str())) {
        eventManager->debug("Certificate mismatch", 1);
        return false;
    }*/

    auto ret = ESPhttpUpdate.update(client, otaHost, otaPort, otaUrl);
    // if successful, ESP will restart
    switch (ret) {
        case HTTP_UPDATE_FAILED:
            eventManager->debug("OTA Update failed: " + ESPhttpUpdate.getLastErrorString(), 1);
            return false;
        case HTTP_UPDATE_NO_UPDATES:
            eventManager->debug("OTA No updates", 1);
            return false;
        case HTTP_UPDATE_OK:
            eventManager->debug("OTA Update successful", 1);  // may not be called since we reboot the ESP
            return true;
    }
    return false;
}
#endif