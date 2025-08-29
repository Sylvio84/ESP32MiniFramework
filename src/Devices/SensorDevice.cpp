#include <Devices/SensorDevice.h>
#include <Managers/ConfigurationManager.h>
#include <ctime>

SensorDevice::SensorDevice(String id, FrameworkContext& ctx)
    : Device(id, ctx)
{
    // Base sensor initialization
    type = "sensor";
}

void SensorDevice::init()
{
    Device::init();
    
    // Load common sensor configuration
    auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
    if (configMgr) {
        // Load common sensor settings
        int intervalSeconds = configMgr->getPreference(id + "_interval", readInterval / 1000);
        readInterval = intervalSeconds * 1000;
        maxConsecutiveErrors = configMgr->getPreference(id + "_max_errors", 3);
    }
    
    // Register common sensor commands
    registerDeviceCommand("read", "Force immediate sensor reading",
        [this](const std::vector<String>&) { 
            forceRead(); 
            return "Reading sensor..."; 
        });
    
    registerDeviceCommand("?", "Get sensor status",
        [this](const std::vector<String>&) { 
            return getStatus(); 
        });
    
    registerDeviceCommand("interval", "Get/set reading interval in seconds",
        [this](const std::vector<String>& params) -> String {
            if (params.size() > 0) {
                int seconds = params[0].toInt();
                if (seconds >= (int)(minReadInterval / 1000)) {
                    setReadInterval(seconds);
                    return "Interval set to " + String(seconds) + " seconds";
                }
                return String("Invalid interval (minimum " + String(minReadInterval / 1000) + " seconds)");
            }
            return "Current interval: " + String(readInterval / 1000) + " seconds";
        });
    
    debug("SensorDevice base initialized", 2);
}

void SensorDevice::loop()
{
    // Check if it's time for a reading
    if (isTimeToRead()) {
        debug("Time to read sensor (interval: " + String(readInterval) + "ms)", 2);
        lastReadTime = millis();
        
        // Perform the actual reading (implemented by derived class)
        if (performReading()) {
            debug("Sensor reading successful", 2);
            handleReadSuccess();
        } else {
            debug("Sensor reading failed", 2);
            handleReadError();
        }
    }
}

bool SensorDevice::isTimeToRead()
{
    unsigned long currentMillis = millis();
    return (currentMillis - lastReadTime >= readInterval);
}

void SensorDevice::handleReadSuccess()
{
    consecutiveErrors = 0;
    sensorError = false;
    lastSuccessfulRead = millis();
    
    debug("Publishing sensor data to MQTT", 2);
    // Publish sensor data (implemented by derived class)
    publishSensorData();
    
    // Publish status
    publishStatus("online");
    publishLastReadTime();
    
    debug("Sensor reading successful", 3);
}

void SensorDevice::handleReadError()
{
    consecutiveErrors++;
    debug("Sensor reading failed (error " + String(consecutiveErrors) + "/" + String(maxConsecutiveErrors) + ")", 2);
    
    if (consecutiveErrors >= maxConsecutiveErrors) {
        if (!sensorError) {
            sensorError = true;
            publishStatus("error");
            debug("Sensor entered error state after " + String(maxConsecutiveErrors) + " consecutive errors", 1);
        }
    }
}

void SensorDevice::publishStatus(const String& status)
{
    // Use triggerEvent to properly propagate to MQTTManager with retain flag
    context->getEventManager()->triggerEvent("mqtt", "publishRetain", {topic + "/status", status});
}

void SensorDevice::publishLastReadTime()
{
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
    // Use triggerEvent to properly propagate to MQTTManager with retain flag
    context->getEventManager()->triggerEvent("mqtt", "publishRetain", {topic + "/lastread", String(timeStr)});
}

void SensorDevice::forceRead()
{
    debug("Forcing sensor read", 2);
    lastReadTime = 0;  // Force read on next loop
}

String SensorDevice::getStatus()
{
    String status = "Sensor Status:\n";
    
    if (sensorError) {
        status += "State: ERROR\n";
        status += "Consecutive errors: " + String(consecutiveErrors) + "\n";
    } else if (lastSuccessfulRead == 0) {
        status += "State: NO DATA\n";
    } else {
        status += "State: OK\n";
    }
    
    // Add sensor-specific status (implemented by derived class)
    status += getSensorStatus();
    
    // Add common status info
    status += "\nInterval: " + String(readInterval / 1000) + "s\n";
    
    long timeSince = getTimeSinceLastRead();
    if (timeSince >= 0) {
        status += "Last read: " + String(timeSince) + "s ago";
    } else {
        status += "Last read: Never";
    }
    
    return status;
}

void SensorDevice::setReadInterval(unsigned long intervalSeconds)
{
    if (intervalSeconds < (minReadInterval / 1000)) {
        intervalSeconds = minReadInterval / 1000;
    }
    readInterval = intervalSeconds * 1000;
    
    // Save to configuration
    auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
    if (configMgr) {
        configMgr->setPreference(id + "_interval", (int)intervalSeconds);
    }
    
    debug("Sensor read interval set to " + String(intervalSeconds) + " seconds", 2);
}

long SensorDevice::getTimeSinceLastRead()
{
    if (lastSuccessfulRead == 0) {
        return -1;
    }
    return (millis() - lastSuccessfulRead) / 1000;
}

bool SensorDevice::processCommand(String command, std::vector<String> params)
{
    debug("Processing sensor command: " + command, 3);
    
    // First try base Device class processing
    if (Device::processCommand(command, params)) {
        return true;
    }
    
    // Common sensor commands are handled via registerDeviceCommand
    // Derived classes can add their specific commands
    
    return false;
}