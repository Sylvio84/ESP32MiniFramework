#include "../include/WiFiManager.h"

EventManager* WiFiManager::eventManager = nullptr;

void WiFiManager::init(bool auto_connect)
{
    prefs.begin("wifi", false);
    this->ssid = prefs.getString("ssid", "");
    this->password = prefs.getString("password", "");
    if (auto_connect)
    {
        this->autoConnect();
    }
}

bool WiFiManager::autoConnect()
{
    if (this->ssid != "" && this->password != "")
    {
        this->connect();
        if (this->connected)
        {
            return true;
        }
    }
    Serial.println("Starting Access Point");
    this->startAccessPoint();
    return false;
}

// Connect to WiFi
bool WiFiManager::connect()
{
    Serial.println("Connecting to: " + this->ssid);
    WiFi.mode(WIFI_STA);

    WiFi.begin(this->ssid.c_str(), this->password.c_str());
    uint timeout = 0;
    uint wait = 100;
    while ((timeout <= CONNECTION_TIMEOUT) && (WiFi.status() != WL_CONNECTED))
    {
        delay(wait);
        if (timeout < 1000)
        {
            wait = 200;
        }
        else
        {
            wait = 1000;
        }
        timeout += wait;
        Serial.print(".");
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Connection failed");
        this->connected = false;
        return false;
    }
    else
    {
        std::vector<String> params;
        params.push_back(WiFi.SSID());
        params.push_back(WiFi.localIP().toString());
        //Serial.println("Connected to WiFi: " + WiFi.SSID());
        //Serial.println("IP address: " + WiFi.localIP().toString());
        this->connected = true;
        eventManager->triggerEvent("WiFi", "Connected", params);
        return true;
    }
}

void WiFiManager::disconnect()
{
    WiFi.disconnect();
    this->connected = false;
    eventManager->triggerEvent("WiFi","Disconnected", {});
}

void WiFiManager::startAccessPoint()
{
    Serial.println("\n\nCreating Hotspot: " + this->ApHostname);
    Serial.println("IP Address: " + this->apIP.toString());
    WiFi.mode(WIFI_AP);
    delay(100);
    WiFi.softAPConfig(this->apIP, this->apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(this->ApHostname);
    eventManager->triggerEvent("WiFi", "AccessPointStarted", {});
}

/*bool WiFiManager::processCommand(String command)
{
    if (command.indexOf("ssid") > -1)
    {
        saveSSID(splitString(command, ' ', 1));
        return true;
    }
    else if (command.indexOf("password") > -1)
    {
        savePassword(splitString(command, ' ', 1));
        return true;
    }
    else if (command.indexOf("connect") > -1)
    {
        this->connect();
        return true;
    }
    else if (command.indexOf("wifi") > -1)
    {
        Serial.println("WiFi status:" + String(this->connected));
        Serial.println("IP: " + WiFi.localIP().toString());
        Serial.println("SSID: \"" + this->ssid + "\"");
        Serial.println("Password: \"" + this->password + "\"");
        return true;
    }
    else if (command.indexOf("hotspot") > -1)
    {
        this->disconnect();
        this->startAccessPoint();
        return true;
    }

    return false;
}*/

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
    prefs.putString("ssid", ssid);
}

void WiFiManager::savePassword(String password)
{
    this->password = password;
    Serial.println("New WiFi password: " + this->password);
    prefs.putString("password", password);
}

String WiFiManager::getDebugInfos()
{
    return "SSID: " + prefs.getString("ssid", "") + "\nPassword: " + prefs.getString("password", "");
}

String WiFiManager::retrieveSSID()
{
    return prefs.getString("ssid", "");
}

String WiFiManager::retrieveIP()
{
    return WiFi.localIP().toString();
}