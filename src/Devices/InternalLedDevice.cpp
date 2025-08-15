#include <Managers/ConfigurationManager.h>
#include <Devices/InternalLedDevice.h>
#include <Managers/EventManager.h>

InternalLedDevice::InternalLedDevice(String id, FrameworkContext& ctx) : OnOffDevice(id, ctx)
{
    name = "Internal Led";
    invertedLogic = true;  // LED interne utilise logique inversée
    debug("InternalLedDevice constructed", 1);
}

void InternalLedDevice::detectLedPin()
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
            pin = 2;  // Default LED pin for most ESP32 boards
            break;
    }
#elif defined(ESP8266)
    pin = 2;
#else
    pin = 2;
#endif
}

void InternalLedDevice::init()
{
    // First detect the LED pin
    detectLedPin();
    debug("LED pin detected: " + String(pin), 1);
    
    // Then call parent init
    OnOffDevice::init();
}

void InternalLedDevice::registerSpecificCommands()
{
    // Register LED-specific commands
    registerDeviceCommand("blink", "Start blink pattern (interval_ms duration_ms)", 
        [this](const std::vector<String>& args) -> String {
            if (args.size() < 2) {
                return String("ERROR: Usage: led:blink <interval_ms> <duration_ms>");
            }
            int interval = args[0].toInt();
            int duration = args[1].toInt();
            if (interval <= 0 || duration <= 0) {
                return String("ERROR: Interval and duration must be positive");
            }
            startBlinkPattern(interval, duration);
            return String("Blink pattern started: " + String(interval) + "ms interval for " + String(duration) + "ms");
        });
    
    // Additional aliases for LED
    auto* cmdMgr = static_cast<CommandManager*>(context->getManager("CommandManager"));
    if (cmdMgr) {
        cmdMgr->registerAlias("led_on", "led:on");
        cmdMgr->registerAlias("led_off", "led:off");
        cmdMgr->registerAlias("led_toggle", "led:toggle");
        cmdMgr->registerAlias("led_status", "led:state");
    }
}

void InternalLedDevice::loop()
{
    // Process blink pattern if active
    processBlinkPattern();
    
    if (active && blinkInterval > 0 && !patternActive) {
        // If the LED is active, blink it
        static unsigned long lastBlink = 0;
        static bool blinkState = false;
        unsigned long currentMillis = millis();
        if (currentMillis - lastBlink >= blinkInterval) {
            lastBlink = currentMillis;
            blinkState = !blinkState;
            writePin(blinkState);
        }
    }
}

void InternalLedDevice::activate(int timeout)
{
    active = true;
    OnOffDevice::activate(timeout);  // Call parent implementation
}

void InternalLedDevice::deactivate()
{
    active = false;
    blinkInterval = 0;  // Reset blink interval when deactivating
    OnOffDevice::deactivate();  // Call parent implementation
}

bool InternalLedDevice::getActive()
{
    debug("LED active state: " + String(active), 1);
    context->getEventManager()->triggerEvent("mqtt", "publishAsap", {topic + "/active", active ? "1" : "0"});
    return active;
}

void InternalLedDevice::startBlinkPattern(int intervalMs, int durationMs)
{
    patternActive = true;
    patternStartTime = millis();
    patternLastToggle = millis();
    patternInterval = intervalMs;
    patternDuration = durationMs;
    patternLedState = false;
    writePin(patternLedState);
}

void InternalLedDevice::processBlinkPattern()
{
    if (!patternActive) {
        return;
    }

    unsigned long currentTime = millis();

    // Check if blink duration has expired
    if (currentTime - patternStartTime >= patternDuration) {
        patternActive = false;
        writePin(false);  // Ensure LED is off at the end
        return;
    }

    // Check if it's time to toggle the LED
    if (currentTime - patternLastToggle >= patternInterval) {
        patternLedState = !patternLedState;
        writePin(patternLedState);
        patternLastToggle = currentTime;
    }
}

bool InternalLedDevice::processMQTTDevice(String topic, String value)
{
    // First check parent class processing
    if (OnOffDevice::processMQTTDevice(topic, value)) {
        return true;
    }
    
    // Then check LED-specific commands
    if (topic == this->topic || topic.endsWith("/" + id)) {
        if (value.startsWith("blink ")) {
            // Parse "blink interval duration"
            String params = value.substring(6);
            int spaceIndex = params.indexOf(' ');
            if (spaceIndex > 0) {
                int interval = params.substring(0, spaceIndex).toInt();
                int duration = params.substring(spaceIndex + 1).toInt();
                if (interval > 0 && duration > 0) {
                    startBlinkPattern(interval, duration);
                    debug("LED blink pattern started via MQTT: " + String(interval) + "ms for " + String(duration) + "ms", 1);
                    return true;
                }
            }
        }
    }
    
    return false;
}