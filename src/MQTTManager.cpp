#include <MQTTManager.h>
#include <ConfigurationManager.h>
#include <EventManager.h>
#include <CommandManager.h>


void MQTTManager::init()
{
    logDebug("MQTTManager init...", 1);

    // Register MQTT commands with CommandManager FIRST
    registerCommands();

    retrieveServer();
    retrievePort();
    retrieveUsername();
    retrievePassword();

    if (server == "") {
        logDebug("No MQTT server configured", 1);
        setInitialized(true);
        return;
    }
    if (username == "") {
        logDebug("No MQTT username configured", 1);
    }
    mqttClient.setServer(server.c_str(), port);

    //mqttClient.setKeepAlive(5);
    //mqttClient.setSocketTimeout(100);

    /*mqttClient.setCallback([this](char *topic, byte *payload, unsigned int length)
                           { eventManager->triggerEvent("mqtt", "message", {topic, String((char *)payload, length)}); });*/
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        // Créer une instance de String avec la bonne longueur
        String payloadString;
        payloadString.reserve(length);  // Réserver de l'espace pour éviter les reallocations
        for (unsigned int i = 0; i < length; ++i) {
            payloadString += (char)payload[i];
        }
        context->getEventManager()->triggerEvent("mqtt", "message", {topic, payloadString});
    });
    
    setInitialized(true);
}

void MQTTManager::loop()
{
    static unsigned long lastMQTTReconnect = 0;
    static unsigned long retry = 0;
    static unsigned long lastMQTTLoop = 0;
    static unsigned long reconnectDelay = 1; // initial delay in seconds
    unsigned long currentMillis = millis();

//  every seconds display status
//  static unsigned long lastStatusDisplay = 0;
//  if (currentMillis - lastStatusDisplay >= 1000) {
//       lastStatusDisplay = currentMillis;
//       eventManager->debug("MQTT status #" + String(status) + ": " + (mqttClient.connected() ? "connected" : "disconnected"), 1);
//  }

    /*if (!wifiClient.available()) {
        eventManager->debug("No WiFi connection, MQTT disabled", 1);
        return;
    }*/

    if (currentMillis - lastPing >= pingInterval) {
        if (mqttClient.connected()) {
            auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
            String hostname = configMgr ? configMgr->getHostname() : "ESP32";
            publish(hostname + "/status", "online", false);
        } else {
            logDebug("MQTT status #" + String(status) + ": " + (mqttClient.connected() ? "connected" : "disconnected"), 1);
        }
        lastPing = currentMillis;
    }

    if (currentMillis - lastMQTTReconnect >= (reconnectDelay * 1000)) {
        if ((status >= 2) && server != "" && !mqttClient.connected()) {
            logDebug("MQTT: Try to connect....", 1);
            if (!reconnect()) {
                retry++;
                reconnectDelay = 1;
                for (unsigned long i = 0; i < retry && reconnectDelay < 60; i++) {
                    reconnectDelay *= 2;
                }
                if (reconnectDelay > 60) {
                    reconnectDelay = 60;
                }
                logDebug("MQTT connection failed, try again in " + String(reconnectDelay) + "s (attempt " + String(retry + 1) + ")", 1);
            } else {
                // Successful connection, reset retry count and delay
                retry = 0;
                reconnectDelay = 1;
                logDebug("MQTT connected successfully", 1);
            }
        }
        lastMQTTReconnect = currentMillis;
    }

    if ((lastMQTTLoop == 0) || (currentMillis - lastMQTTLoop >= 25)) {
        if (mqttClient.loop()) {
            // Si la connexion est maintenue et qu'on avait des échecs précédents
            if (retry > 0) {
                retry = 0;
                reconnectDelay = 1;
                logDebug("MQTT connection restored", 1);
            }
        }
        lastMQTTLoop = currentMillis;
    }
}

void MQTTManager::setStatus(uint status)
{
    this->status = status;
}

bool MQTTManager::isConnected()
{
    return mqttClient.connected();
}

bool MQTTManager::reconnect()
{
    if (!mqttClient.connected()) {
        context->getEventManager()->triggerEvent("mqtt", "ConnectionInProgress", {});
        logDebug("Attempting MQTT connection...", 1);
        auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
        String hostname = configMgr ? configMgr->getHostname() : "ESP32";
        if (mqttClient.connect(hostname.c_str(), username.c_str(), password.c_str())) {
            context->getEventManager()->triggerEvent("mqtt", "Connected", {this->server});
            logDebug("MQTT connected (hostname = " + hostname + ")", 1);
            for (const auto& topic : subscriptions) {
                logDebug("Process subscription " + topic, 2);
                subscribe(topic);
            }
            for (const auto& pub : publications) {
                logDebug("Publishing stored publication: " + pub.first + " = " + pub.second, 2);
                publish(pub.first, pub.second);
                removePublication(pub.first);
            }
            return true;
        } else {
            context->getEventManager()->triggerEvent("mqtt", "ConnectionFailed", {});
            logDebug("MQTT error: " + String(mqttClient.state()), 2);
            IPAddress serverIP;
            if (WiFi.hostByName(hostname.c_str(), serverIP)) {
                logDebug("Server IP: " + serverIP.toString(), 2);
            } else {
                logDebug("DNS lookup failed", 1);
            }
            WiFiClient testClient;
            if (testClient.connect(serverIP, 1883)) {
                logDebug("TCP connection successful", 2);
                testClient.stop();
            } else {
                logDebug("TCP connection failed", 2);
            }
            return false;
        }
    }
    return true;
}

void MQTTManager::publish(String topic, String payload, bool enableDebug)
{
    if (enableDebug) {
        logDebug("Publishing to " + topic + ": " + payload, 2);
    }
    if (!mqttClient.connected()) {
        logDebug("MQTT not connected, can't publish: " + topic + " = " + payload, 1);
        return;
    }
    mqttClient.publish(topic.c_str(), payload.c_str());
}

void MQTTManager::subscribe(String topic)
{
    logDebug("Subscribing to " + topic, 2);
    mqttClient.subscribe(topic.c_str());
}

void MQTTManager::unsubscribe(String topic)
{
    logDebug("Unsubscribing from " + topic, 2);
    mqttClient.unsubscribe(topic.c_str());
}

void MQTTManager::saveServer(String server)
{
    this->server = server;
    logDebug("Saving MQTT server: " + server, 1);
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    if (configMgr) configMgr->setPreference("mq_serv", server);
}

void MQTTManager::savePort(int port)
{
    this->port = port;
    logDebug("Saving MQTT port: " + String(port), 1);
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    if (configMgr) configMgr->setPreference("mq_port", port);
}

void MQTTManager::saveUsername(String username)
{
    this->username = username;
    logDebug("Saving MQTT username: " + username, 1);
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    if (configMgr) configMgr->setPreference("mq_user", username);
}

void MQTTManager::savePassword(String password)
{
    this->password = password;
    logDebug("Saving MQTT password: " + password, 1);
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    if (configMgr) configMgr->setPreference("mq_pass", password);
}

String MQTTManager::retrieveServer()
{
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    server = configMgr ? configMgr->getPreference("mq_serv", server) : server;
    return server;
}

int MQTTManager::retrievePort()
{
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    port = configMgr ? configMgr->getPreference("mq_port", port) : port;
    return port;
}

String MQTTManager::retrieveUsername()
{
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    username = configMgr ? configMgr->getPreference("mq_user", this->username) : this->username;
    return username;
}

String MQTTManager::retrievePassword()
{
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    password = configMgr ? configMgr->getPreference("mq_pass", this->password) : this->password;
    return password;
}

String MQTTManager::getDebugInfos()
{
    return "Server: " + retrieveServer() + "\nPort: " + retrievePort() + "\nUsername: " + retrieveUsername() + "\nPassword: " + retrievePassword();
}

bool MQTTManager::onEvent(const String& type, const String& event, const std::vector<String>& params)
{
    logDebug("Processing MQTT event: " + type + " / " + event, 3);
    for (const auto& param : params) {
        logDebug("Param: " + param, 3);
    }
    if (type == "mqtt") {
        if (event.startsWith("@")) {
            return onCommand(event.substring(1), params);
        } else if (event == "connected") {
            logDebug("Connected to MQTT server: " + (params.size() > 0 ? params[0] : "unknown"), 1);
            return true;
        } else if (event == "message") {
            // MQTT message received - this should be handled by MainController
            // since it needs to call processMQTT()
            return false; // Let MainController handle this
        } else if (event == "subscribe") {
            logDebug("process event subscribe to " + params[0], 3);
            if (params.size() > 0) {
                addSubscription(params[0]);
                subscribe(params[0]);
            } else {
                logDebug("Missing topic", 1);
            }
            return true;
        } else if (event == "unsubscribe") {
            if (params.size() > 0) {
                removeSubscription(params[0]);
                unsubscribe(params[0]);
            } else {
                logDebug("Missing topic", 1);
            }
            return true;
        } else if (event == "publish") {
            if (params.size() > 1) {
                publish(params[0], params[1]);
            } else {
                logDebug("Missing topic or payload", 1);
            }
            return true;
        } else if (event == "publishAsap") {
            if (params.size() > 1) {
                if (isConnected()) {
                    publish(params[0], params[1]);
                } else {
                    storePublication(params[0], params[1]);
                }
            } else {
                logDebug("Missing topic or payload", 1);
            }
            return true;
        } else if (event == "removePublication") {
            if (params.size() > 0) {
                removePublication(params[0]);
            } else {
                logDebug("Missing topic", 1);
            }
            return true;
        }
    }
    
    return false; // Event not handled
}

bool MQTTManager::onCommand(const String& command, const std::vector<String>& params)
{
    logDebug("Processing MQTT command: " + command, 3);
    if (command == "server") {
        if (params.size() > 0) {
            saveServer(params[0]);
            logDebug("Server set to: " + params[0], 0);
        } else {
            logDebug("Server: " + retrieveServer(), 0);
        }
    } else if (command == "port") {
        if (params.size() > 0) {
            savePort(params[0].toInt());
            logDebug("Port set to: " + params[0], 0);
        } else {
            logDebug("Port: " + String(retrievePort()), 0);
        }
    } else if (command == "user") {
        if (params.size() > 0) {
            saveUsername(params[0]);
            logDebug("Username set to: " + params[0], 0);
        } else {
            logDebug("Username: " + retrieveUsername(), 0);
        }
    } else if (command == "pass") {
        if (params.size() > 0) {
            savePassword(params[0]);
            logDebug("Password set to: " + params[0], 0);
        } else {
            logDebug("Password: " + retrievePassword(), 0);
        }
    } else if (command == "status") {
        if (isConnected()) {
            logDebug("MQTT: Connected", 0);
        } else {
            logDebug("MQTT: Not connected", 0);
        }
    } else if (command == "connect") {
        reconnect();
    } else if (command == "subscribe") {
        if (params.size() > 0) {
            addSubscription(params[0]);
            subscribe(params[0]);
        } else {
            logDebug("Missing topic", 1);
        }
    } else if (command == "unsubscribe") {
        if (params.size() > 0) {
            removeSubscription(params[0]);
            unsubscribe(params[0]);
        } else {
            logDebug("Missing topic", 1);
        }
    } else if (command == "publish") {
        if (params.size() > 1) {
            publish(params[0], params[1]);
        } else {
            logDebug("Missing topic or payload", 1);
        }
    } else if (command == "subscriptions") {
        for (const auto& topic : getSubscriptions()) {
            logDebug("- Subscription: " + topic, 0);
        }
    } else if (command == "debug") {
        logDebug(getDebugInfos(), 0);
    } else {
        return false;
    }
    return true;
}

void MQTTManager::registerCommands()
{
    auto* cmdMgr = static_cast<CommandManager*>(context ? context->getManager("CommandManager") : nullptr);
    if (!cmdMgr) return;

    // MQTT connection commands
    cmdMgr->registerCommand(Command(
        "mqtt", "server", "Get/Set MQTT server hostname",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                saveServer(args[0]);
                return "MQTT server set to: " + args[0];
            }
            return "MQTT server: " + retrieveServer();
        }
    ));

    cmdMgr->registerCommand(Command(
        "mqtt", "port", "Get/Set MQTT server port",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                savePort(args[0].toInt());
                return "MQTT port set to: " + args[0];
            }
            return "MQTT port: " + String(retrievePort());
        }
    ));

    cmdMgr->registerCommand(Command(
        "mqtt", "user", "Get/Set MQTT username",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                saveUsername(args[0]);
                return "MQTT username set to: " + args[0];
            }
            return "MQTT username: " + retrieveUsername();
        }
    ));

    cmdMgr->registerCommand(Command(
        "mqtt", "pass", "Get/Set MQTT password",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                savePassword(args[0]);
                return "MQTT password set to: " + args[0];
            }
            return "MQTT password: " + retrievePassword();
        }
    ));

    cmdMgr->registerCommand(Command(
        "mqtt", "status", "Show MQTT connection status",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            return isConnected() ? "MQTT: Connected to " + server + ":" + String(port) 
                                : "MQTT: Not connected";
        }
    ));

    cmdMgr->registerCommand(Command(
        "mqtt", "connect", "Connect to MQTT server",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            return reconnect() ? "MQTT: Connection successful" : "MQTT: Connection failed";
        }
    ));

    cmdMgr->registerCommand(Command(
        "mqtt", "subscribe", "Subscribe to MQTT topic",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                addSubscription(args[0]);
                subscribe(args[0]);
                return "Subscribed to: " + args[0];
            }
            return "Usage: mqtt:subscribe <topic>";
        }
    ));

    cmdMgr->registerCommand(Command(
        "mqtt", "unsubscribe", "Unsubscribe from MQTT topic",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            if (args.size() > 0) {
                removeSubscription(args[0]);
                unsubscribe(args[0]);
                return "Unsubscribed from: " + args[0];
            }
            return "Usage: mqtt:unsubscribe <topic>";
        }
    ));

    cmdMgr->registerCommand(Command(
        "mqtt", "publish", "Publish to MQTT topic",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            if (args.size() > 1) {
                publish(args[0], args[1]);
                return "Published to " + args[0] + ": " + args[1];
            }
            return "Usage: mqtt:publish <topic> <payload>";
        }
    ));

    cmdMgr->registerCommand(Command(
        "mqtt", "subscriptions", "List active MQTT subscriptions",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            String result = "MQTT subscriptions:\n";
            auto subs = getSubscriptions();
            if (subs.empty()) {
                result += "  (none)";
            } else {
                for (const auto& topic : subs) {
                    result += "  - " + topic + "\n";
                }
            }
            return result;
        }
    ));

    cmdMgr->registerCommand(Command(
        "mqtt", "info", "Show detailed MQTT information",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            String result = "MQTT Configuration:\n";
            result += "  Server: " + retrieveServer() + "\n";
            result += "  Port: " + String(retrievePort()) + "\n";
            result += "  Username: " + retrieveUsername() + "\n";
            result += "  Password: " + String(retrievePassword().isEmpty() ? "(none)" : "***") + "\n";
            result += "  Status: " + String(isConnected() ? "Connected" : "Disconnected") + "\n";
            result += "  Subscriptions: " + String(getSubscriptions().size());
            return result;
        }
    ));


    // Register common aliases
    cmdMgr->registerAlias("ms", "mqtt:status");
    cmdMgr->registerAlias("mi", "mqtt:info");
    cmdMgr->registerAlias("mc", "mqtt:connect");
}

bool MQTTManager::addSubscription(String topic)
{
    if (topic.length() == 0) {
        return false;
    }
    if (std::find(subscriptions.begin(), subscriptions.end(), topic) == subscriptions.end()) {
        subscriptions.push_back(topic);
        return true;
    }
    return false;
}

bool MQTTManager::removeSubscription(String topic)
{
    if (topic.length() == 0) {
        return false;
    }
    auto it = std::find(subscriptions.begin(), subscriptions.end(), topic);
    if (it != subscriptions.end()) {
        logDebug("Removing subscription: " + topic, 3);
        subscriptions.erase(it);
        return true;
    }
    return false;
}

std::vector<String> MQTTManager::getSubscriptions()
{
    return subscriptions;
}

bool MQTTManager::storePublication(String topic, String payload)
{
    logDebug("Storing MQTT publication: " + topic + " = " + payload, 3);
    auto it = publications.find(topic);
    publications[topic] = payload;
    return it != publications.end();
}

bool MQTTManager::removePublication(String topic)
{
    logDebug("Removing MQTT publication: " + topic, 3);
    auto it = publications.find(topic);
    if (it != publications.end()) {
        publications.erase(it);
        return true;
    }
    return false;
}

#ifndef DISABLE_ESPUI
void MQTTManager::initEspUI()
{
    logDebug("Init MQTTManager ESPUI", 2);

    auto callback = std::bind(&MQTTManager::EspUiCallback, this, std::placeholders::_1, std::placeholders::_2);

    auto mqttTab = ESPUI.addControl(Tab, "", "MQTT");
    mqttServerInput = ESPUI.addControl(Text, "Server", server, Peterriver, mqttTab, callback);
    mqttPortInput = ESPUI.addControl(Number, "Port", String(port), Peterriver, mqttTab, callback);
    mqttUserInput = ESPUI.addControl(Text, "User", username, Peterriver, mqttTab, callback);
    mqttPasswordInput = ESPUI.addControl(Text, "Password", "", Peterriver, mqttTab, callback);

    ESPUI.setInputType(mqttPasswordInput, "password");

    auto mqttSave = ESPUI.addControl(Button, "Save", "Save", Peterriver, mqttTab, callback);

    ESPUI.setEnabled(mqttSave, true);

    auto mqttReconnect = ESPUI.addControl(Button, "Reconnect", "MQTTReconnect", Peterriver, mqttTab, callback);
    ESPUI.setEnabled(mqttReconnect, true);
}

void MQTTManager::EspUiCallback(Control* sender, int type)
{
    logDebug(
        "MQTT ESPUI callback: sender.value = " + sender->value + " sender.id = " + sender->id + " sender.type = " + sender->type + "  / type = " + String(type),
        2);
    if (type == B_DOWN) {
        return;
    }
    if (sender->value == "MQTTSave") {
        std::vector<String> params1;
        params1.push_back(ESPUI.getControl(mqttServerInput)->value);
        context->getEventManager()->triggerEvent("ESPUI", "MQTTSaveServer", params1);

        std::vector<String> params2;
        params2.push_back(ESPUI.getControl(mqttPortInput)->value);
        context->getEventManager()->triggerEvent("ESPUI", "MQTTSavePort", params2);

        std::vector<String> params3;
        params3.push_back(ESPUI.getControl(mqttUserInput)->value);
        context->getEventManager()->triggerEvent("ESPUI", "MQTTSaveUser", params3);

        String password = ESPUI.getControl(mqttPasswordInput)->value;
        if (password.length() > 0) {
            std::vector<String> params4;
            params4.push_back(password);
            context->getEventManager()->triggerEvent("ESPUI", "MQTTSavePassword", params4);
        }
    } else if (sender->value == "MQTTReconnect") {
        context->getEventManager()->triggerEvent("ESPUI", "MQTTReconnect", {});
    }
}
#endif
