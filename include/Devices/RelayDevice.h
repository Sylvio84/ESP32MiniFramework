#ifndef RELAYDEVICE_H
#define RELAYDEVICE_H

#include <Arduino.h>
#include <Configuration.h>
#include <Devices/Device.h>

class RelayDevice : public Device
{

  public:
    //int pin = 0;
    int pin = 12;
    int state = LOW;

    int timeoutId = 0;

    RelayDevice(String id, Configuration& config, EventManager& eventMgr, TimeManager& timeMgr) : Device(id, config, eventMgr, timeMgr)
    {
        //pin = config.getValue("relay_pin", 0);
        addCommand("1", std::bind(&RelayDevice::activate, this));
        addCommand("0", std::bind(&RelayDevice::deactivate, this));
        addCommand("?", std::bind(&RelayDevice::getState, this));
    }

    // Implémentation des méthodes virtuelles
    void init() override
    {
        Device::init();
        eventManager->debug("Initializing RelayDevice #" + id + " with pin: " + String(pin), 1);
        pinMode(pin, OUTPUT);
        state = LOW;
        digitalWrite(pin, state);
        getState();
    }

    void loop() override {}

    void activate()
    {
        eventManager->debug("Activating relay", 1);
        digitalWrite(pin, HIGH);
        state = HIGH;
        getState();
    }

    void deactivate()
    {
        eventManager->debug("Deactivating relay", 1);
        digitalWrite(pin, LOW);
        state = LOW;
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
        } else if (command == "state") {
            getState();
            eventManager->debug("State: " + String(state), 0);
            return true;
        }
        return false;
    }

    bool processMQTT(String topic, String value) override
    {
        Device::processMQTT(topic, value);
        eventManager->debug("RelayDevice #" + id + " MQTT message: " + topic + " = " + value, 3);

        if (topic == this->topic && isInteger(value)) {
            int duration = value.toInt();
            if (duration > 1) {
                eventManager->debug("Setting relay duration: " + value + " seconds", 2);
                activate();
                timeoutId = timeManager.setTimeout([this] { deactivate(); }, duration * 1000);
            }
            return true;
        }

        return false;
    }
};

#endif  // RELAYDEVICE_H
