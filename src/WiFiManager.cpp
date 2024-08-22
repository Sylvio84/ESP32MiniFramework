#include "../include/WiFiManager.h"

EventManager *WiFiManager::eventManager = nullptr;

void WiFiManager::init(bool auto_connect)
{
    Serial.println("WiFiManager init...");
    this->ssid = retrieveSSID();
    this->password = retrievePassword();
    if (auto_connect)
    {
        this->autoConnect();
    }
}

void WiFiManager::loop()
{
    static unsigned long lastMillis = 0;
    unsigned long currentMillis = millis();

    if (connectionStatus == 0)
    {
        this->connect();
        lastMillis = currentMillis;
        connectionStatus = 1;
    }

    unsigned long wait = (tryCount + 1) * checkDelay;

    if (currentMillis - lastMillis >= wait)
    {
        if (connectionStatus == 1) // Connection in progress
        {
            if (WiFi.status() != WL_CONNECTED)
            {
                this->connected = false;
                tryCount++;
                std::vector<String> params;
                params.push_back(String(tryCount));
                eventManager->triggerEvent("WiFi", "ConnectionInProgress", params);
                if (tryCount >= 10)
                {
                    tryCount = 0;
                    if (keepConnected)
                    {
                        Serial.println("Connection failed, retrying");
                    }
                    else
                    {
                        connectionStatus = 2;
                        Serial.println("Connection failed");
                        eventManager->triggerEvent("WiFi", "ConnectionFailed", params);
                        this->startAccessPoint();
                    }
                }
                else
                {
                    Serial.print("*");
                }
            }
            else
            {
                connected = true;
                keepConnected = true;
                connectionStatus = 10;
                tryCount = 0;
                std::vector<String> params;
                params.push_back(WiFi.SSID());
                params.push_back(WiFi.localIP().toString());
                eventManager->triggerEvent("WiFi", "Connected", params);
            }
        }
        else if (connectionStatus == 10) // Connected
        {
            if (WiFi.status() != WL_CONNECTED)
            {
                connectionStatus = 3;
                this->connected = false;
                Serial.println("Connection lost");
                eventManager->triggerEvent("WiFi", "ConnectionLost", {});
            }
        }
        else if (connectionStatus == 3) // Connection lost
        {
            if (WiFi.status() == WL_CONNECTED)
            {
                connectionStatus = 10;
                this->connected = true;
                eventManager->triggerEvent("WiFi", "ConnectionRecovered", {});
            }
        }
        lastMillis = currentMillis;
    }
}

bool WiFiManager::autoConnect()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi AutoConnect...");
        if (this->ssid == "")
        {
            Serial.println("No SSID, starting access point");
            this->startAccessPoint();
            return false;
        }
        else
        {
            connectionStatus = 0;
            connect();
            return WiFi.status() == WL_CONNECTED;
        }
    }
    else
    {
        return true;
    }
}

bool WiFiManager::connect()
{
    connectionStatus = 1;
    Serial.println("Connecting to: " + this->ssid + " password: " + this->password);
    WiFi.mode(WIFI_STA);

    WiFi.begin(this->ssid.c_str(), this->password.c_str());
    delay(100);

    if (WiFi.status() == WL_CONNECTED)
    {
        this->connected = true;
        connectionStatus = 10;
        return true;
    }
    else
    {
        return false;
    }
}

void WiFiManager::disconnect()
{
    WiFi.disconnect();
    this->connected = false;
    connectionStatus = 4;
    eventManager->triggerEvent("WiFi", "Disconnected", {});
}

void WiFiManager::startAccessPoint()
{
    Serial.println("\n\nCreating Hotspot: " + this->ApHostname);
    Serial.println("IP Address: " + this->apIP.toString());
    WiFi.mode(WIFI_AP);
    delay(100);
    WiFi.softAPConfig(this->apIP, this->apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(this->ApHostname);
    connectionStatus = 5;
    eventManager->triggerEvent("WiFi", "AccessPointStarted", {});
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
    return config.getPreference("wf_ssid", this->ssid);
}

String WiFiManager::retrievePassword()
{
    return config.getPreference("wf_pass", this->password);
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
    switch (connectionStatus)
    {
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
    if (name == "mac")
    {
        return WiFi.macAddress();
    }
    if (name == "ip")
    {
        return WiFi.localIP().toString();
    }
    if (name == "ssid")
    {
        return WiFi.SSID();
    }
    if (name == "rssi")
    {
        return String(WiFi.RSSI());
    }
    if (name == "status")
    {
        return String(WiFi.status());
    }
    if (name == "gateway")
    {
        return WiFi.gatewayIP().toString();
    }
    if (name == "subnet")
    {
        return WiFi.subnetMask().toString();
    }
    if (name == "dns")
    {
        return WiFi.dnsIP().toString();
    }
    if (name == "hostname")
    {
        return WiFi.hostname();
    }
    if (name == "bssid")
    {
        return WiFi.BSSIDstr();
    }
    if (name == "channel")
    {
        return String(WiFi.channel());
    }
    if (name == "psk")
    {
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
    if (save)
    {
        this->saveSSID(this->ssid);
    }
}

String WiFiManager::getNetworkInfo(int n, String name)
{
    if (name == "ssid")
    {
        return WiFi.SSID(n);
    }
    if (name == "bssid")
    {
        return WiFi.BSSIDstr(n);
    }
    if (name == "channel")
    {
        return String(WiFi.channel(n));
    }
    if (name == "rssi")
    {
        return String(WiFi.RSSI(n));
    }
    if (name == "encryption")
    {
        return String(WiFi.encryptionType(n));
    }
    return "";
}