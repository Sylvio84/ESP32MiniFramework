#ifndef INTERNALLEDDEVICE_H
#define INTERNALLEDDEVICE_H

#include <Arduino.h>
#include <Configuration.h>
#include <Devices/Device.h>
#include <esp_chip_info.h>

class InternalLedDevice : public Device
{
  public:
    int pin = -1;

    bool active = false;
    bool ledState = false;
    int blinkInterval = 100;  // Blink interval in milliseconds

    InternalLedDevice(String id, Configuration& config, EventManager& eventMgr, TimeManager& timeMgr) : Device(id, config, eventMgr, timeMgr)
    {
        name = "Internal Led";
        addCommand("1", std::bind(&InternalLedDevice::activate, this));
        addCommand("0", std::bind(&InternalLedDevice::deactivate, this));
        addCommand("?", std::bind(&InternalLedDevice::getState, this));
    }

    void detectLedPin() {
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);

        switch (chip_info.model) {
          case CHIP_ESP32:
            pin = 2;  // DevKit, NodeMCU, etc.
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
          default:
            pin = 2;  // Fallback default
            break;
        }
    }

    void setPin(int ledPin)
    {
        pin = ledPin;
        eventManager->debug("LED pin set to: " + String(pin), 1);
    }

    void init() override
    {
        Device::init();
        detectLedPin();
        eventManager->debug("Detected LED pin: " + String(pin), 1);
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);  // Ensure LED is off initially
        eventManager->debug("ESP32C3SuperMiniLedDevice initialized on pin " + String(pin), 1);
    }

    void loop() override
    {
        if (active && blinkInterval > 0) {
            // If the LED is active, blink it every second
            static unsigned long lastBlink = 0;
            unsigned long currentMillis = millis();
            //eventManager->debug("ESP32C3SuperMiniLedDevice loop running: " + String(currentMillis), 3);
            if (currentMillis - lastBlink >= blinkInterval) {
                lastBlink = currentMillis;
                toggle();  // Toggle the LED state
            }
        }
    }

    void activate()
    {
        active = true;
        digitalWrite(pin, LOW);
        ledState = true;
        eventManager->debug("LED ON", 2);
    }

    void deactivate()
    {
        active = false;
        digitalWrite(pin, HIGH);
        ledState = false;
        eventManager->debug("LED OFF", 2);
    }

    bool getState()
    {
        int currentState = digitalRead(pin);
        eventManager->debug("LED state: " + String(currentState), 1);
        return currentState == HIGH;
    }

    void toggle()
    {
        ledState = !ledState;
        if (ledState) {
            digitalWrite(pin, HIGH);
            eventManager->debug("LED toggled OFF", 3);
        } else {
            digitalWrite(pin, LOW);
            eventManager->debug("LED toggled ON", 3);
        }
    }

    bool processCommand(String command, std::vector<String> params) override
    {
        eventManager->debug("Processing led command: " + command, 3);
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
};

#endif  // INTERNALLEDDEVICE_H