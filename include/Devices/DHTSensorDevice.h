#ifndef DHTSENSORDEVICE_H
#define DHTSENSORDEVICE_H

#include <Arduino.h>
#include <DHT.h>
#include <Devices/SensorDevice.h>

// DHT sensor types
#define DHT11_TYPE 11
#define DHT22_TYPE 22

/**
 * @class DHTSensorDevice
 * @brief DHT temperature and humidity sensor handler for ESP32 Mini Framework
 * 
 * This class manages DHT11/DHT22/AM2302 sensors for temperature and humidity monitoring.
 * Inherits common sensor functionality from SensorDevice base class.
 * 
 * @details
 * Supported sensor models:
 * - DHT11: Basic temperature (0 to 50°C) and humidity (20-80%) sensor
 * - DHT22/AM2302: Higher precision (-40 to 80°C) and humidity (0-100%) sensor
 * 
 * Features:
 * - Temperature reading in Celsius with optional Fahrenheit conversion
 * - Humidity reading in percentage (relative humidity)
 * - Heat index calculation (apparent temperature)
 * - Data validation and error detection
 * - MQTT publishing of sensor values
 * 
 * MQTT Topics (in addition to base SensorDevice topics):
 * - {base_topic}/temperature : Current temperature in °C
 * - {base_topic}/humidity    : Current humidity in %
 * - {base_topic}/heatindex   : Calculated heat index in °C
 * 
 * Configuration (via preferences):
 * - {id}_pin      : GPIO pin number (default: GPIO_NUM_4)
 * - {id}_type     : Sensor type - 11 or 22 (default: 22)
 * - {id}_interval : Reading interval in seconds (default: 60)
 * 
 * @note Minimum reading interval: 2 seconds (sensor limitation)
 * @note DHT sensors require pull-up resistor (usually built into modules)
 * @note Inherits interval, status, and error handling from SensorDevice
 * 
 * @author ESP32 Mini Framework
 * @version 2.0.0
 */
class DHTSensorDevice : public SensorDevice
{
  private:
    // DHT specific configuration
    int SENSOR_TYPE = DHT22_TYPE;

    // Sensor readings
    float lastTemperature = NAN;
    float lastHumidity = NAN;
    float lastHeatIndex = NAN;

    // DHT library instance
    DHT* dhtSensor = nullptr;

    // Initialization timing
    unsigned long initStartTime = 0;
    bool sensorReady = false;
    static const unsigned long INIT_DELAY_MS = 2000;  // 2 seconds stabilization

    // Internal methods
    float computeHeatIndex(float temperature, float humidity);
    bool validateReading(float temperature, float humidity);

  protected:
    // Implement abstract methods from SensorDevice
    bool performReading() override;
    void publishSensorData() override;
    String getSensorStatus() override;
    bool isTimeToRead() override;

  public:
    // Constructor
    DHTSensorDevice(String id, FrameworkContext& ctx);

    // Destructor
    ~DHTSensorDevice();

    // Override parent methods
    void init() override;
    bool processCommand(String command, std::vector<String> params) override;

    // DHT specific public methods
    float getTemperature() { return lastTemperature; }
    float getHumidity() { return lastHumidity; }
    float getHeatIndex() { return lastHeatIndex; }
};

#endif  // DHTSENSORDEVICE_H