#include <Devices/BinarySensorDevice.h>
#include <Managers/ConfigurationManager.h>
#include <ctime>

BinarySensorDevice::BinarySensorDevice(String id, FrameworkContext& ctx) 
    : SensorDevice(id, ctx)
{
    name = "BinarySensor";
    type = "binary_sensor";
    minReadInterval = 50;  // Minimum 50ms for binary sensors
}

void BinarySensorDevice::init()
{
    // Initialize base sensor
    SensorDevice::init();
    
    // Get configuration manager
    auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
    if (configMgr) {
        // Load binary sensor specific configuration
        sensorPin = configMgr->getPreference(id + "_pin", GPIO_NUM_20);
        triggerState = configMgr->getPreference(id + "_trigger", HIGH);
    }
    
    pinMode(sensorPin, INPUT);
    
    // Register binary sensor specific commands
    registerDeviceCommand("activate", "Activate binary sensor",
        [this](const std::vector<String>&) { 
            activate(); 
            return "Binary sensor activated"; 
        });
    
    registerDeviceCommand("deactivate", "Deactivate binary sensor",
        [this](const std::vector<String>&) { 
            deactivate(); 
            return "Binary sensor deactivated"; 
        });
    
    registerDeviceCommand("toggle", "Toggle binary sensor state",
        [this](const std::vector<String>&) { 
            return "Binary sensor toggled to: " + String(toggle()); 
        });
    
    registerDeviceCommand("detection", "Get detection status",
        [this](const std::vector<String>&) { 
            return "Detection: " + String(getDetection() ? "YES" : "NO"); 
        });
    
    // Shortcuts
    registerDeviceCommand("1", "Activate", 
        [this](const std::vector<String>&) { activate(); return "OK"; });
    registerDeviceCommand("0", "Deactivate",
        [this](const std::vector<String>&) { deactivate(); return "OK"; });
    
    debug("BinarySensorDevice initialized on pin " + String(sensorPin), 1);
}

void BinarySensorDevice::loop()
{
    unsigned long currentMillis = millis();
    
    // Fast polling for binary state changes (50ms)
    if (currentMillis - lastPollTime >= pollInterval) {
        lastPollTime = currentMillis;
        
        if (activated && digitalRead(sensorPin) == triggerState) {
            if (detection == LOW) {
                activateDetection();
                publishSensorData();  // Immediate publish on detection
            }
        } else {
            deactivateDetection();
        }
    }
    
    // Regular sensor reading interval for status updates
    SensorDevice::loop();
}

bool BinarySensorDevice::performReading()
{
    // For binary sensor, reading is always successful if activated
    if (!activated) {
        debug("Binary sensor is deactivated", 3);
        return false;
    }
    
    // Update detection state
    bool currentState = (digitalRead(sensorPin) == triggerState);
    if (currentState != (detection == HIGH)) {
        if (currentState) {
            activateDetection();
        } else {
            deactivateDetection();
        }
    }
    
    return true;
}

void BinarySensorDevice::publishSensorData()
{
    // Publish detection state
    processEvent("mqtt", "publishAsap", {topic + "/detection", detection == HIGH ? "1" : "0"});
    
    // Publish activation state
    processEvent("mqtt", "publish", {topic + "/state", activated ? "1" : "0"});
    
    // If there was a recent detection, publish timestamp
    if (lastDetection > 0) {
        time_t now;
        time(&now);
        lastDetectionTime = *localtime(&now);
        String datetime = String(lastDetectionTime.tm_year + 1900) + "-" + 
                         String(lastDetectionTime.tm_mon + 1) + "-" + 
                         String(lastDetectionTime.tm_mday) + " " + 
                         String(lastDetectionTime.tm_hour) + ":" + 
                         String(lastDetectionTime.tm_min) + ":" + 
                         String(lastDetectionTime.tm_sec);
        
        processEvent("mqtt", "publish", {topic + "/last", datetime});
    }
}

String BinarySensorDevice::getSensorStatus()
{
    String status = "";
    status += "Type: Binary Sensor\n";
    status += "Pin: GPIO" + String(sensorPin) + "\n";
    status += "Trigger: " + String(triggerState == HIGH ? "HIGH" : "LOW") + "\n";
    status += "Activated: " + String(activated ? "YES" : "NO") + "\n";
    status += "Detection: " + String(detection == HIGH ? "YES" : "NO") + "\n";
    
    if (lastDetection > 0) {
        float secondsAgo = (millis() - lastDetection) / 1000.0;
        status += "Last detection: " + String(secondsAgo, 1) + "s ago";
    } else {
        status += "Last detection: Never";
    }
    
    return status;
}

void BinarySensorDevice::activate()
{
    debug("Activating binary sensor", 1);
    activated = true;
    getState();
}

void BinarySensorDevice::deactivate()
{
    debug("Deactivating binary sensor", 1);
    activated = false;
    detection = LOW;  // Clear detection when deactivated
    getState();
}

int BinarySensorDevice::toggle()
{
    if (activated) {
        deactivate();
    } else {
        activate();
    }
    return activated;
}

void BinarySensorDevice::getState()
{
    debug("Binary sensor state: " + String(activated ? "active" : "inactive"), 2);
    processEvent("mqtt", "publishAsap", {topic + "/state", activated ? "1" : "0"});
}

bool BinarySensorDevice::getDetection()
{
    return detection == HIGH;
}

float BinarySensorDevice::getLastDetection()
{
    if (lastDetection == 0) {
        return -1;
    }
    return (millis() - lastDetection) / 1000.0;
}

void BinarySensorDevice::activateDetection()
{
    debug("Binary sensor triggered - detection active", 2);
    detection = HIGH;
    lastDetection = millis();
}

void BinarySensorDevice::deactivateDetection()
{
    detection = LOW;
}

bool BinarySensorDevice::processCommand(String command, std::vector<String> params)
{
    debug("Processing binary sensor command: " + command, 3);
    
    // First try parent class processing (includes base sensor commands)
    if (SensorDevice::processCommand(command, params)) {
        return true;
    }
    
    // Binary sensor specific commands are handled via registerDeviceCommand
    return false;
}

bool BinarySensorDevice::processMQTTMessage(String topic, String value)
{
    debug("BinarySensorDevice #" + id + " MQTT message: " + topic + " = " + value, 3);

    if (topic == this->topic && isInteger(value)) {
        int duration = value.toInt();
        if (duration > getLastDetection()) {
            publishSensorData();
        }
        return true;
    }

    return false;
}

bool BinarySensorDevice::isInteger(const String& s)
{
    if (s.length() == 0) return false;
    
    for (unsigned int i = 0; i < s.length(); i++) {
        if (i == 0 && s[i] == '-') continue;
        if (!isDigit(s[i])) return false;
    }
    return true;
}