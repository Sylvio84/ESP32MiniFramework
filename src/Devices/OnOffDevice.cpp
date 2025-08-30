#include <Devices/OnOffDevice.h>
#include <Managers/ConfigurationManager.h>
#include <Managers/EventManager.h>

OnOffDevice::OnOffDevice(String id, FrameworkContext& ctx) : Device(id, ctx)
{
}

void OnOffDevice::writePin(bool logicalState)
{
    if (pin < 0) {
        debug("Pin not set", 1);
        return;
    }

    if (invertedLogic) {
        digitalWrite(pin, logicalState ? LOW : HIGH);
    } else {
        digitalWrite(pin, logicalState ? HIGH : LOW);
    }
}

void OnOffDevice::init()
{
    // Call parent init first (handles name, registerDeviceCommands, ESPUI, MQTT subscription)
    Device::init();

    // Load pin configuration using new system (default is pin variable)
    pin = loadPin("pin", pin);
    
    // Register pin for listing
    registerPin(pin, "output", name + " control pin");
    
    // Initialize pin if set
    if (pin >= 0) {
        pinMode(pin, OUTPUT);
        state = false;
        writePin(false);
        debug("Pin " + String(pin) + " initialized as OUTPUT", 1);
    }

    // Register commands
    registerBaseCommands();
    registerSpecificCommands();

    // Publish initial state
    publishState();
}

void OnOffDevice::registerBaseCommands()
{
    // Base device commands are already registered in Device::init()
    // Just register ON/OFF specific commands here

    // Register ON/OFF commands
    registerDeviceCommand("on", "Turn on (optional: timeout in ms)", [this](const std::vector<String>& args) -> String {
        int timeout = 0;
        if (args.size() > 0) {
            timeout = args[0].toInt();
        }
        activate(timeout);
        if (timeout > 0) {
            return String("Turned ON for " + String(timeout) + "ms");
        }
        return String("Turned ON");
    });

    registerDeviceCommand("off", "Turn off", [this](const std::vector<String>& args) -> String {
        deactivate();
        return String("Turned OFF");
    });

    registerDeviceCommand("toggle", "Toggle state", [this](const std::vector<String>& args) -> String {
        toggle();
        return String("Toggled to " + String(state ? "ON" : "OFF"));
    });

    registerDeviceCommand("state", "Show current state", [this](const std::vector<String>& args) -> String {
        String result = "Status:\n";
        result += "  Pin: " + String(pin) + "\n";
        result += "  State: " + String(state ? "ON" : "OFF") + "\n";
        result += "  Inverted Logic: " + String(invertedLogic ? "Yes" : "No") + "\n";
        result += "  Topic: " + topic;
        publishState();
        return result;
    });

    registerDeviceCommand("setpin", "Set device pin", [this](const std::vector<String>& args) -> String {
        if (args.size() == 0) {
            return String("ERROR: Usage: " + id + ":setpin <pin_number>");
        }
        int newPin = args[0].toInt();
        if (newPin < 0 || newPin > 40) {
            return String("ERROR: Invalid pin number");
        }
        setPin(newPin);

        // Save to configuration using new system
        savePin("pin", newPin);
        
        // Update pin registration for listing
        registerPin(newPin, "output", name + " control pin");

        // Reinitialize with new pin
        pinMode(pin, OUTPUT);
        writePin(state);
        return String("Pin set to " + String(pin));
    });

    // Register convenient aliases
    /*
    auto* cmdMgr = static_cast<CommandManager*>(context->getManager("CommandManager"));
    if (cmdMgr) {
        // Numeric aliases
        cmdMgr->registerAlias("0", id + ":off");
        cmdMgr->registerAlias("1", id + ":on");
        cmdMgr->registerAlias("?", id + ":state");
    }*/

    debug("Base ON/OFF commands registered", 1);
}


void OnOffDevice::loop()
{
    // Base implementation - override in derived classes if needed
}

void OnOffDevice::activate(int timeout)
{
    debug("Activating", 1);
    state = true;
    writePin(true);
    publishState();

    auto* timeMgr = static_cast<TimeManager*>(context->getManager("TimeManager"));
    if (timeout > 1 && timeMgr) {
        // Cancel previous timeout if exists
        if (timeoutId > 1) {
            timeMgr->clearTimeout(timeoutId);
        }
        // Start a timer to deactivate after the specified timeout
        timeoutId = timeMgr->setTimeout(
            [this]() {
                deactivate();
                timeoutId = 0;
            },
            timeout);
        debug("Activated with timeout: " + String(timeout) + "ms", 2);
    } else {
        debug("Activated without timeout", 2);
    }
}

void OnOffDevice::deactivate()
{
    debug("Deactivating", 1);
    state = false;
    writePin(false);
    publishState();

    // Cancel any pending timeout
    if (timeoutId > 0) {
        auto* timeMgr = static_cast<TimeManager*>(context->getManager("TimeManager"));
        if (timeMgr) {
            timeMgr->clearTimeout(timeoutId);
            timeoutId = 0;
        }
    }
}

void OnOffDevice::toggle()
{
    if (state) {
        deactivate();
    } else {
        activate();
    }
}

bool OnOffDevice::getState()
{
    return state;
}

void OnOffDevice::setState(bool newState)
{
    if (newState) {
        activate(0);
    } else {
        deactivate();
    }
}

bool OnOffDevice::isOn()
{
    return state;
}

void OnOffDevice::publishState()
{
    context->getEventManager()->triggerEvent("mqtt", "publishRetain", {topic + "/status", state ? "1" : "0"});
}

void OnOffDevice::setPin(int newPin)
{
    pin = newPin;
    debug("Pin set to: " + String(pin), 1);
}

void OnOffDevice::setInvertedLogic(bool inverted)
{
    invertedLogic = inverted;
    debug("Inverted logic set to: " + String(inverted ? "Yes" : "No"), 1);
}

bool OnOffDevice::processCommand(String command, std::vector<String> params)
{
    debug("Processing command: " + command, 3);

    // First try base device commands
    if (Device::processCommand(command, params)) {
        return true;
    }

    // Legacy command support
    if (command == "activate") {
        if (params.size() > 0) {
            int timeout = params[0].toInt();
            activate(timeout);
        } else {
            activate(0);
        }
        return true;
    } else if (command == "deactivate") {
        deactivate();
        return true;
    } else if (command == "toggle") {
        toggle();
        return true;
    } else if (command == "state") {
        publishState();
        return true;
    }

    return false;
}

bool OnOffDevice::processMQTTDevice(String topic, String value)
{
    debug("Checking MQTT message: " + topic + " = " + value, 3);

    // Check if topic matches our device topic
    if (topic == this->topic || topic.endsWith("/" + id)) {
        if (value == "?" || value == "status") {
            publishState();
            return true;
        } else if (value == "toggle") {
            toggle();
            debug("Toggled via MQTT", 1);
            return true;
        } else if (isInteger(value)) {
            int val = value.toInt();
            if (val == 0) {
                deactivate();
                debug("Deactivated via MQTT (0)", 1);
                return true;
            } else if (val == 1) {
                activate(0);
                debug("Activated via MQTT (1)", 1);
                return true;
            } else if (val > 1) {
                activate(val);  // Activate for val milliseconds
                debug("Activated via MQTT for " + String(val) + "ms", 1);
                return true;
            }
        } else if (value == "on" || value == "ON") {
            activate(0);
            debug("Activated via MQTT", 1);
            return true;
        } else if (value == "off" || value == "OFF") {
            deactivate();
            debug("Deactivated via MQTT", 1);
            return true;
        } else {
            debug("Unknown MQTT command: " + value, 1);
        }
    }

    return false;
}

void OnOffDevice::onProgramStart()
{
    activate(0);
}

void OnOffDevice::onProgramEnd()
{
    deactivate();
}

bool OnOffDevice::isInteger(const String& str)
{
    if (str.length() == 0)
        return false;
    for (unsigned int i = 0; i < str.length(); i++) {
        if (!isDigit(str.charAt(i))) {
            return false;
        }
    }
    return true;
}