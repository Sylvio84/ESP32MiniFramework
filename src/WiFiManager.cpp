#include "../include/WiFiManager.h"

EventManager* WiFiManager::eventManager = nullptr;

void WiFiManager::init(bool auto_connect)
{
    Serial.println("WiFiManager init...");
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
                if (tryCount >= 10) {
                    tryCount = 0;
                    if (keepConnected) {
                        Serial.println("Connection failed, retrying");
                    } else {
                        connectionStatus = 2;
                        Serial.println("Connection failed");
                        eventManager->triggerEvent("wifi", "failed", params);
                        this->startAccessPoint();
                    }
                } else {
                    Serial.print("*");
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
                Serial.println("Connection lost");
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
            Serial.println("SSID: " + retrieveSSID());
        }
    } else if (command == "pass") {
        if (params.size() > 0) {
            this->savePassword(params[0]);
        } else {
            Serial.println("Password: " + retrievePassword());
        }
    } else if (command == "reset") {
        this->saveSSID("");
        this->savePassword("");
    } else if (command == "autoconnect") {
        this->autoConnect();
    } else if (command == "status" || command == "") {
        Serial.println("Connected: " + String(isConnected()));
        Serial.println("Status: " + getStatus());
        Serial.println("SSID: " + getInfo("ssid"));
        Serial.println("IP: " + getInfo("ip"));
        Serial.println("MAC: " + getInfo("mac"));
        Serial.println("RSSI: " + getInfo("rssi"));
    } else if (command == "debug") {
        Serial.println(getDebugInfos());
    } else if (command == "keep") {
        if (keepConnection()) {
            Serial.println("Keep connection: ON");
        } else {
            Serial.println("Keep connection: OFF");
        }
    } else if (command == "scan") {
        this->scanNetworks();
    } else if (command == "network") {
        if (params.size() > 0) {
            this->setNetwork(params[0].toInt(), true);
        } else {
            Serial.println("Current network: " + getSSID());
        }
    } else {
        return false;
    }
    return true;
}

bool WiFiManager::autoConnect()
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi AutoConnect...");
        if (this->ssid == "") {
            Serial.println("No SSID, starting access point");
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
    Serial.println("Connecting to: " + this->ssid + "...");
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
    Serial.println("\n\nCreating Hotspot: " + this->ApHostname);
    Serial.println("IP Address: " + this->apIP.toString());
    WiFi.mode(WIFI_AP);
    delay(100);
    WiFi.softAPConfig(this->apIP, this->apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(this->ApHostname);
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
    Serial.println("New WiFi SSID: " + this->ssid);
    config.setPreference("wf_ssid", ssid);
}

void WiFiManager::savePassword(String password)
{
    this->password = password;
    Serial.println("New WiFi password: " + this->password);
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
    if (name == "hostname") {
        return WiFi.hostname();
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

/*void WiFiManager::initEspUI(ESPUIManager *espUIManager) 
{
    espUIManager->initWiFiTab();
}*/

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

void WiFiManager::EspUiCallback(Control *sender, int type)
{
    eventManager->debug("WiFi ESPUI callback: sender.value = " + sender->value + " sender.id = " + sender->id + " sender.type = " + sender->type + "  / type = " + String(type), 2);
    if (type == B_DOWN)
    {
        return;
    }
    
    if (sender->value == "WiFiConnect")
    {
        eventManager->triggerEvent("ESPUI", "WiFiConnect", {});
    }
    else if (sender->value == "WiFiSave")
    {
        std::vector<String> params1;
        params1.push_back(ESPUI.getControl(ssidInput)->value);
        eventManager->triggerEvent("ESPUI", "WiFiSaveSSID", params1);

        String password = ESPUI.getControl(passwordInput)->value;
        if (password.length() > 0)
        {
            std::vector<String> params2;
            params2.push_back(password);
            eventManager->triggerEvent("ESPUI", "WiFiSavePassword", params2);
        }
    }

}
