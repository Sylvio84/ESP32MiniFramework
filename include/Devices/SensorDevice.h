#ifndef SENSORDEVICE_H
#define SENSORDEVICE_H

#include <Arduino.h>
#include <Devices/Device.h>

/**
 * @class SensorDevice
 * @brief Abstract base class for all sensor devices in ESP32 Mini Framework
 * 
 * This class provides common functionality for sensor devices including:
 * - Periodic reading management with configurable intervals
 * - Error handling and consecutive error tracking
 * - Common MQTT publishing patterns
 * - Standardized command interface
 * 
 * @details
 * Common features for all sensors:
 * - Automatic periodic readings
 * - Force read command
 * - Status reporting
 * - Error detection and recovery
 * - MQTT status publishing
 * 
 * Common MQTT Topics:
 * - {base_topic}/status : Sensor status (online/offline/error)
 * - {base_topic}/lastread : Timestamp of last successful reading
 * 
 * Common Commands:
 * - read     : Force immediate sensor reading
 * - ?        : Get current sensor status
 * - interval : Get/set reading interval in seconds
 * 
 * @note Derived classes must implement:
 * - performReading() : Actual sensor reading logic
 * - publishSensorData() : Publish sensor-specific data
 * - getSensorStatus() : Return sensor-specific status string
 * 
 * @author ESP32 Mini Framework
 * @version 1.0.0
 */
class SensorDevice : public Device
{
protected:
    // Configuration
    int sensorPin = -1;
    unsigned long readInterval = 60000;  // Default 60 seconds
    unsigned long minReadInterval = 1000;  // Default minimum 1 second
    
    // Sensor state
    unsigned long lastReadTime = 0;
    unsigned long lastSuccessfulRead = 0;
    bool sensorError = false;
    int consecutiveErrors = 0;
    int maxConsecutiveErrors = 3;
    
    // Protected methods for derived classes
    
    /**
     * @brief Perform the actual sensor reading
     * @return true if reading was successful, false otherwise
     */
    virtual bool performReading() = 0;
    
    /**
     * @brief Publish sensor-specific data via MQTT
     */
    virtual void publishSensorData() = 0;
    
    /**
     * @brief Get sensor-specific status information
     * @return Status string with sensor details
     */
    virtual String getSensorStatus() = 0;
    
    /**
     * @brief Handle successful sensor reading
     */
    virtual void handleReadSuccess();
    
    /**
     * @brief Handle failed sensor reading
     */
    virtual void handleReadError();
    
    /**
     * @brief Publish sensor status via MQTT
     * @param status Status string to publish
     */
    void publishStatus(const String& status);
    
    /**
     * @brief Publish last read timestamp via MQTT
     */
    void publishLastReadTime();
    
    /**
     * @brief Check if enough time has passed for next reading
     * @return true if it's time to read, false otherwise
     */
    bool isTimeToRead();
    
public:
    // Constructor
    SensorDevice(String id, FrameworkContext& ctx);
    
    // Virtual method implementations
    void init() override;
    void loop() override;
    bool processCommand(String command, std::vector<String> params) override;
    
    // Public methods
    
    /**
     * @brief Force an immediate sensor reading
     */
    virtual void forceRead();
    
    /**
     * @brief Get current sensor status
     * @return Status string
     */
    String getStatus();
    
    /**
     * @brief Set the reading interval
     * @param intervalSeconds Interval in seconds
     */
    virtual void setReadInterval(unsigned long intervalSeconds);
    
    /**
     * @brief Get the current reading interval
     * @return Interval in seconds
     */
    unsigned long getReadInterval() { return readInterval / 1000; }
    
    /**
     * @brief Check if sensor is in error state
     * @return true if sensor has errors
     */
    bool hasError() { return sensorError; }
    
    /**
     * @brief Get time since last successful reading
     * @return Time in seconds, -1 if never read
     */
    long getTimeSinceLastRead();
};

#endif  // SENSORDEVICE_H