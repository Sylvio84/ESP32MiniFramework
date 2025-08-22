#ifndef BINARYSENSORDEVICE_H
#define BINARYSENSORDEVICE_H

#include <Arduino.h>
#include <Devices/SensorDevice.h>

/**
 * @class BinarySensorDevice
 * @brief Binary sensor device handler for ESP32 Mini Framework
 * 
 * This class manages binary sensors that provide HIGH/LOW digital signals.
 * It monitors GPIO pin state changes and publishes detection events via MQTT.
 * Inherits common sensor functionality from SensorDevice base class.
 * 
 * @details
 * Supported sensor types:
 * - PIR motion sensors (HC-SR501, AM312, etc.)
 * - Magnetic door/window sensors (reed switches)
 * - Optical barriers (infrared beam break sensors)
 * - Proximity sensors with digital output
 * - Any sensor providing binary HIGH/LOW signals
 * 
 * Features:
 * - Configurable GPIO pin and trigger state (HIGH/LOW)
 * - Real-time detection with configurable polling interval
 * - MQTT publishing for state changes
 * - Timestamp tracking for last detection event
 * - Software enable/disable capability
 * 
 * MQTT Topics (in addition to base SensorDevice topics):
 * - {base_topic}/detection : Published when detection occurs (value: "1")
 * - {base_topic}/state     : Current activation state ("0" or "1")
 * 
 * Configuration:
 * - {id}_pin     : GPIO pin number (default: GPIO_NUM_20)
 * - {id}_trigger : Trigger state HIGH or LOW (default: HIGH)
 * 
 * @note Default polling rate: 50ms
 * @note Inherits interval, status, and error handling from SensorDevice
 * 
 * @author ESP32 Mini Framework
 * @version 2.0.0
 */
class BinarySensorDevice : public SensorDevice
{
private:
    // Binary sensor specific configuration
    int triggerState = HIGH;
    int detection = LOW;
    unsigned long lastDetection = 0;
    std::tm lastDetectionTime;
    bool activated = true;  // Software enable/disable
    
    // Polling configuration
    unsigned long pollInterval = 50;  // 50ms polling rate
    unsigned long lastPollTime = 0;
    
    // Utility methods
    bool isInteger(const String& s);
    
protected:
    // Implement abstract methods from SensorDevice
    bool performReading() override;
    void publishSensorData() override;
    String getSensorStatus() override;

public:
    // Constructor
    BinarySensorDevice(String id, FrameworkContext& ctx);
    
    // Override parent methods
    void init() override;
    void loop() override;
    bool processCommand(String command, std::vector<String> params) override;
    
    // Binary sensor specific methods
    void activate();
    void deactivate();
    int toggle();
    void getState();
    bool getDetection();
    float getLastDetection();
    void activateDetection();
    void deactivateDetection();
    
    // MQTT processing
    bool processMQTTMessage(String topic, String value);
};

#endif  // BINARYSENSORDEVICE_H