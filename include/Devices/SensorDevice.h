#ifndef SENSORDEVICE_H
#define SENSOREVICE_H

#include <Arduino.h>
#include <Configuration.h>
#include <Devices/Device.h>

class SensorDevice : public Device
{

  public:
    int SENSOR_PIN = GPIO_NUM_20;
    int triggerState = HIGH;
    
    int detection = LOW;
    unsigned long lastDetection = 0;
    std::tm lastDetectionTime;

    bool state = false;

    SensorDevice(String id, Configuration& config, EventManager& eventMgr, TimeManager& timeMgr) : Device(id, config, eventMgr, timeMgr)
    {
        //pin = config.getValue("relay_pin", 0);
        addCommand("1", std::bind(&SensorDevice::activate, this));
        addCommand("0", std::bind(&SensorDevice::deactivate, this));
        addCommand("?", std::bind(&SensorDevice::getState, this));
    }

    // Implémentation des méthodes virtuelles
    void init() override
    {
        Device::init();
        pinMode(GPIO_NUM_20, INPUT);
        triggerState = HIGH;
        getDetection();
    }

    void loop() override {
        static unsigned long sensorLoop = 0;
        static unsigned long publishLoop = 0;
        unsigned long currentMillis = millis();

        if (currentMillis - sensorLoop >= 50) {
            sensorLoop = currentMillis;
            if (digitalRead(SENSOR_PIN) == triggerState) {
                activateDetection();
                publish();
            } else {
                deactivateDetection();
            }
        }

        if (currentMillis - publishLoop >= 1000) {
            publishLoop = currentMillis;
            if (getDetection()) {
                publish();
            }
        }
    }

    void publish(bool modeTime = false)
    {
        if (modeTime) {
            String datetime = lastDetectionTime.tm_year + 1900 + "-" + lastDetectionTime.tm_mon + "-" + lastDetectionTime.tm_mday + " " + lastDetectionTime.tm_hour + ":" + lastDetectionTime.tm_min + ":" + lastDetectionTime.tm_sec;
            eventManager->triggerEvent("mqtt", "publishAsap", {topic + "/last", datetime});
        } else {
            eventManager->triggerEvent("mqtt", "publishAsap", {topic + "/detection", "1"});
        }
    }

    void activate()
    {
        eventManager->debug("Activating sensor", 1);
        state = true;
        getState();
    }

    void deactivate()
    {
        eventManager->debug("Deactivating sensor", 1);
        state = false;
        getState();
    }

    int toggle()
    {
        if (state == HIGH) {
            deactivate();
        } else {
            activate();
        }
        return state;
    }

    void getState()
    {
        eventManager->debug("Relay state: " + String(state), 1);
        eventManager->triggerEvent("mqtt", "publishAsap", {topic + "/status", state == HIGH ? "1" : "0"});
    }

    bool getDetection()
    {
        eventManager->debug("Detection: " + String(detection), 0);
        return detection == HIGH;
    }

    float getLastDetection()
    {
        if (lastDetection == 0) {
            return -1;
        }
        return (millis() - lastDetection) / 1000;
        lastDetectionTime = timeManager.getDateTime();
    }

    void activateDetection()
    {
        eventManager->debug("Motion detected", 1);
        detection = HIGH;
        lastDetection = millis();
    }

    void deactivateDetection()
    {
        detection = LOW;
    }

    bool processCommand(String command, std::vector<String> params) override
    {
        eventManager->debug("Processing relay command: " + command, 3);
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
        } else if (command == "detection") {
            getDetection();
            return true;
        }
        return false;
    }

    bool processMQTT(String topic, String value) override
    {
        Device::processMQTT(topic, value);
        eventManager->debug("SensorDevice #" + id + " MQTT message: " + topic + " = " + value, 3);

        if (topic == this->topic && isInteger(value)) {
            int duration = value.toInt();
            if (duration > getLastDetection()) {
                publish(true);
            }
            return true;
        }

        return false;
    }
};

#endif  // SENSORDEVICE_H
