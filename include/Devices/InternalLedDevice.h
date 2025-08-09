#ifndef INTERNALLEDDEVICE_H
#define INTERNALLEDDEVICE_H

#include <Arduino.h>
#include <Devices/Device.h>
#include <esp_chip_info.h>
#include <CommandManager.h>
#include <Command.h>

class InternalLedDevice : public Device
{
  public:
    int pin = -1;

    bool active = false;
    bool ledState = false;
    int blinkInterval = 100;  // Blink interval in milliseconds

    InternalLedDevice(String id, FrameworkContext& ctx) : Device(id, ctx)
    {
        name = "Internal Led";
        
        // DELAY hostname setup until init() - Configuration may not be ready yet
        topic = "ESP32/" + id;  // Temporary topic
        debug("constructed - will setup topic in init()", 1);
        addCommand("1", std::bind(&InternalLedDevice::activate, this));
        addCommand("0", std::bind(&InternalLedDevice::deactivate, this));
        addCommand("?", std::bind(&InternalLedDevice::getState, this));
    }

    void detectLedPin()
    {
#if defined(ESP32)
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);

        switch (chip_info.model) {
            case CHIP_ESP32:
                pin = 2;
                break;
            case CHIP_ESP32S2:
                pin = 18;
                break;
            case CHIP_ESP32S3:
                pin = 48;
                break;
            case CHIP_ESP32C3:
                pin = 8;
                break;
            case CHIP_ESP32H2:
                pin = 2;
                break;
            default:
                pin = LED_BUILTIN;  // Fallback to default LED pin
                break;
        }

#elif defined(ESP8266)
        pin = 2;  // NodeMCU, Wemos D1 Mini, etc.
#else
        pin = 2;  // Unknown platform fallback
#endif

        return;
    }

    void setPin(int ledPin)
    {
        pin = ledPin;
        debug("LED pin set to: " + String(pin), 1);
    }

    void init() override
    {
        // SKIP Device::init() to avoid Configuration access
        // Device::init(); // COMMENTED OUT to prevent Configuration access
        
        // Use simple static topic to avoid Configuration dependency
        topic = "ESP32/" + id;
        debug("LED device initialized with static topic: " + topic, 1);
        
        detectLedPin();
        if (pin < 0) {
            debug("LED pin not set or detected", 1);
            return;
        }
        debug("initialized on pin " + String(pin), 3);
        pinMode(pin, OUTPUT);
        ledOff();
        
        // Register LED commands
        registerCommands();
    }
    
    void registerCommands()
    {
        auto* cmdMgr = static_cast<CommandManager*>(context->getManager("CommandManager"));
        if (!cmdMgr) {
            debug("CommandManager not available for LED commands", 1);
            return;
        }
        
        // LED device commands in 'led' namespace
        cmdMgr->registerCommand(Command(
            "led", "on", "Turn LED on",
            CommandSource::Any, false,
            [this](const std::vector<String>& args) -> String {
                try {
                    activate();
                    return String("LED turned ON");
                } catch (...) {
                    return String("ERROR: LED control failed");
                }
            }
        ));
        
        cmdMgr->registerCommand(Command(
            "led", "off", "Turn LED off",
            CommandSource::Any, false,
            [this](const std::vector<String>& args) -> String {
                try {
                    deactivate();
                    return String("LED turned OFF");
                } catch (...) {
                    return String("ERROR: LED control failed");
                }
            }
        ));
        
        cmdMgr->registerCommand(Command(
            "led", "toggle", "Toggle LED state",
            CommandSource::Any, false,
            [this](const std::vector<String>& args) -> String {
                try {
                    toggle();
                    return String("LED toggled");
                } catch (...) {
                    return String("ERROR: LED control failed");
                }
            }
        ));
        
        cmdMgr->registerCommand(Command(
            "led", "status", "Show LED status",
            CommandSource::Any, false,
            [this](const std::vector<String>& args) -> String {
                try {
                    bool currentState = getState();
                    String result = "LED Status:\n";
                    result += "  Pin: " + String(pin) + "\n";
                    result += "  State: " + String(currentState ? "ON" : "OFF") + "\n";
                    result += "  Active (blinking): " + String(active ? "Yes" : "No") + "\n";
                    result += "  Blink interval: " + String(blinkInterval) + "ms";
                    return result;
                } catch (...) {
                    return String("ERROR: Could not read LED status");
                }
            }
        ));
        
        // Register global aliases for convenience
        cmdMgr->registerAlias("led_on", "led:on");
        cmdMgr->registerAlias("led_off", "led:off");
        cmdMgr->registerAlias("led_toggle", "led:toggle");
        cmdMgr->registerAlias("led_status", "led:status");
        
        debug("LED commands registered in 'led' namespace", 1);
    }
    
    void updateTopicFromConfiguration() 
    {
        // Call this later to update topic with real hostname
        try {
            auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
            if (configMgr) {
                String configHostname = configMgr->getHostname();
                if (!configHostname.isEmpty()) {
                    topic = configHostname + "/" + id;
                    debug("topic updated to: " + topic, 1);
                    
                    // Subscribe to MQTT topic for LED control
                    if (subscribeMQTT(topic)) {
                        debug("subscribed to MQTT topic: " + topic, 1);
                    } else {
                        debug("failed to subscribe to MQTT topic: " + topic, 1);
                    }
                }
            }
        } catch (...) {
            debug("Could not update topic from configuration - keeping static", 1);
        }
    }

    void loop() override
    {
        if (active && blinkInterval > 0) {
            // If the LED is active, blink it every second
            static unsigned long lastBlink = 0;
            unsigned long currentMillis = millis();
            //context->getService<EventManager>()->debug("ESP32C3SuperMiniLedDevice loop running: " + String(currentMillis), 3);
            if (currentMillis - lastBlink >= blinkInterval) {
                lastBlink = currentMillis;
                toggle();  // Toggle the LED state
            }
        }
    }

    void activate()
    {
        active = true;
        ledOn();
        debug("LED ON", 2);
    }

    void deactivate()
    {
        active = false;
        ledOff();
        debug("LED OFF", 2);
    }

    bool getState()
    {
        int currentState = digitalRead(pin);
        debug("LED state: " + String(currentState), 1);
        context->getEventManager()->triggerEvent("mqtt", "publishAsap", {topic + "/status", ledState ? "1" : "0"});
        return currentState == HIGH;
    }

    void toggle()
    {
        if (ledState) {
            ledOff();
            debug("LED toggled OFF", 3);
        } else {
            ledOn();
        }
    }

    void ledOff()
    {
        if (pin < 0) {
            debug("LED pin not set or detected", 1);
            return;
        }
        digitalWrite(pin, HIGH);
        ledState = false;
        debug("LED turned OFF", 2);
    }

    void ledOn()
    {
        if (pin < 0) {
            debug("LED pin not set or detected", 1);
            return;
        }
        digitalWrite(pin, LOW);
        ledState = true;
        debug("LED turned ON", 2);
    }

    bool processCommand(String command, std::vector<String> params) override
    {
        debug("Processing led command: " + command, 3);
        Device::processCommand(command, params);
        if (command == "activate") {
            activate();
            return true;
        } else if (command == "deactivate") {
            deactivate();
            return true;
        } else if (command == "toggle") {
            toggle();
            return true;
        } else if (command == "state") {
            getState();
            debug("State: " + String(state), 0);
            return true;
        }
        return false;
    }

    bool processMQTT(String topic, String value) override
    {
        Device::processMQTT(topic, value);
        debug("MQTT message: " + topic + " = " + value, 2);
        
        // Handle LED-specific MQTT messages
        if (topic.endsWith("/" + id)) {  // e.g., hostname/led
            if (value == "1" || value == "on" || value == "ON") {
                activate();
                debug("LED activated via MQTT", 1);
                return true;
            } else if (value == "0" || value == "off" || value == "OFF") {
                deactivate();
                debug("LED deactivated via MQTT", 1);
                return true;
            } else if (value == "?" || value == "status") {
                getState();  // This publishes current state
                return true;
            } else if (value == "toggle") {
                toggle();
                debug("LED toggled via MQTT", 1);
                return true;
            }
        }
        
        return false;  // Message not handled
    }

    void onProgramStart() override { activate(); }

    void onProgramEnd() override { deactivate(); }
};

#endif  // INTERNALLEDDEVICE_H