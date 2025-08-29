#include <Devices/DHTSensorDevice.h>
#include <Managers/ConfigurationManager.h>

DHTSensorDevice::DHTSensorDevice(String id, FrameworkContext& ctx)
    : SensorDevice(id, ctx)
{
    name = "DHTSensor";
    type = "dht_sensor";
    minReadInterval = 2000;  // Minimum 2 seconds for DHT sensors
    dhtSensor = nullptr;  // Initialize to nullptr
    
    // Set default topic like other devices do
    auto* configMgr = static_cast<ConfigurationManager*>(ctx.getManager("ConfigurationManager"));
    if (configMgr) {
        String hostname = configMgr->getHostname();
        topic = hostname + "/" + id;
    }
}

DHTSensorDevice::~DHTSensorDevice()
{
    if (dhtSensor) {
        delete dhtSensor;
        dhtSensor = nullptr;
    }
}

void DHTSensorDevice::init()
{
    // Initialize base sensor
    SensorDevice::init();
    
    // Get configuration manager
    auto* configMgr = static_cast<ConfigurationManager*>(context->getManager("ConfigurationManager"));
    if (configMgr) {
        // Load DHT specific configuration
        #ifdef ESP32
            sensorPin = configMgr->getPreference(id + "_pin", GPIO_NUM_4);
        #else
            sensorPin = configMgr->getPreference(id + "_pin", 4);  // ESP8266 uses plain integers
        #endif
        SENSOR_TYPE = configMgr->getPreference(id + "_type", DHT22_TYPE);
    }
    
    // Create and initialize DHT sensor with Adafruit library
    dhtSensor = new DHT(sensorPin, SENSOR_TYPE);
    dhtSensor->begin();
    
    // Mark initialization start time for non-blocking delay
    initStartTime = millis();
    sensorReady = false;
    
    debug("DHTSensorDevice initializing with Adafruit library - Type: DHT" + String(SENSOR_TYPE) + " on pin " + String(sensorPin), 1);
    debug("Sensor will be ready in " + String(INIT_DELAY_MS) + "ms", 2);
    debug("MQTT topic configured: " + topic, 1);
    
    // Set default interval if not already configured
    if (readInterval < minReadInterval) {
        setReadInterval(30);  // Default 60 seconds
    }
    debug("Read interval set to: " + String(readInterval) + "ms", 1);
}

bool DHTSensorDevice::isTimeToRead()
{
    if (SensorDevice::isTimeToRead()) {
        return true;
    }
    if (!sensorReady) {
        unsigned long elapsed = millis() - initStartTime;
        if (elapsed >= INIT_DELAY_MS) {
            sensorReady = true;
            debug("DHT sensor is now ready for readings", 1);
            return true;  // First reading after stabilization
        }
    }
    return false;
}

bool DHTSensorDevice::performReading()
{
    debug("DHT22 performReading called", 2);
    
    if (!dhtSensor) {
        debug("DHT sensor not initialized", 2);
        return false;
    }
    
    // Check if sensor stabilization period has passed
    if (!sensorReady) {
        unsigned long elapsed = millis() - initStartTime;
        if (elapsed < INIT_DELAY_MS) {
            debug("Sensor still stabilizing (" + String(INIT_DELAY_MS - elapsed) + "ms remaining)", 3);
            return false;
        }
        sensorReady = true;
        debug("DHT sensor ready for readings", 2);
    }
    
    // Read temperature and humidity using Adafruit library
    float humidity = dhtSensor->readHumidity();
    float temperature = dhtSensor->readTemperature();
    
    debug("DHT raw readings - T: " + String(temperature) + "°C, H: " + String(humidity) + "%", 2);
    
    // Validate readings
    if (!validateReading(temperature, humidity)) {
        debug("Invalid sensor readings - T:" + String(temperature) + " H:" + String(humidity), 2);
        return false;
    }
    
    // Store values
    lastHumidity = humidity;
    lastTemperature = temperature;
    lastHeatIndex = dhtSensor->computeHeatIndex(temperature, humidity, false);  // false = Celsius
    
    debug("DHT reading successful - T:" + String(temperature, 1) + "°C H:" + String(humidity, 1) + "% HI:" + String(lastHeatIndex, 1) + "°C", 3);
    
    return true;
}

void DHTSensorDevice::publishSensorData()
{
    debug("Publishing DHT data to MQTT - Topic: " + topic, 2);
    debug("Temperature: " + String(lastTemperature, 1) + "°C", 2);
    debug("Humidity: " + String(lastHumidity, 1) + "%", 2);
    debug("Heat Index: " + String(lastHeatIndex, 1) + "°C", 2);
    
    // Use triggerEvent with retain flag for sensor data
    context->getEventManager()->triggerEvent("mqtt", "publishRetain", {topic + "/temperature", String(lastTemperature, 1)});
    context->getEventManager()->triggerEvent("mqtt", "publishRetain", {topic + "/humidity", String(lastHumidity, 1)});
    context->getEventManager()->triggerEvent("mqtt", "publishRetain", {topic + "/heatindex", String(lastHeatIndex, 1)});
}

String DHTSensorDevice::getSensorStatus()
{
    String status = "";
    status += "Type: DHT" + String(SENSOR_TYPE) + "\n";
    status += "Pin: GPIO" + String(sensorPin) + "\n";
    
    if (!isnan(lastTemperature)) {
        status += "Temperature: " + String(lastTemperature, 1) + "°C\n";
        status += "Humidity: " + String(lastHumidity, 1) + "%\n";
        status += "Heat Index: " + String(lastHeatIndex, 1) + "°C";
    } else {
        status += "No valid readings yet";
    }
    
    return status;
}

// readDHTData and expectPulse methods are no longer needed
// The Adafruit DHT library handles all low-level communication

float DHTSensorDevice::computeHeatIndex(float temperature, float humidity)
{
    // Use Adafruit library's heat index calculation if available
    if (dhtSensor) {
        return dhtSensor->computeHeatIndex(temperature, humidity, false);  // false = Celsius
    }
    
    // Fallback to simple calculation if sensor not initialized
    if (temperature < 27) {
        return temperature;
    }
    
    return 0.5 * (temperature + 61.0 + ((temperature - 68.0) * 1.2) + (humidity * 0.094));
}

bool DHTSensorDevice::validateReading(float temperature, float humidity)
{
    // Check for NaN
    if (isnan(temperature) || isnan(humidity)) {
        return false;
    }
    
    // Check ranges based on sensor type
    if (SENSOR_TYPE == DHT11_TYPE) {
        if (temperature < 0 || temperature > 50) return false;
        if (humidity < 20 || humidity > 80) return false;
    } else {  // DHT22
        if (temperature < -40 || temperature > 80) return false;
        if (humidity < 0 || humidity > 100) return false;
    }
    
    return true;
}

bool DHTSensorDevice::processCommand(String command, std::vector<String> params)
{
    debug("Processing DHT sensor command: " + command, 3);
    
    // All commands are handled by base SensorDevice class
    return SensorDevice::processCommand(command, params);
}