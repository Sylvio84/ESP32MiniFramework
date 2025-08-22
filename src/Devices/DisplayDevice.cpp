#include "Devices/DisplayDevice.h"

// Default empty implementation
void DisplayDevice::displaySystemMessage(uint8_t messageType) {
    // Base implementation does nothing
    // Derived classes should override this method if they want to display system messages
}

// Process system events and trigger displaySystemMessage
void DisplayDevice::processEvent(String type, String event, std::vector<String> params) {
    
    if (type == "system") {
        if (event == "init_complete") {
            Serial.println("[DisplayDevice] Calling displaySystemMessage(INIT_OK)");
            displaySystemMessage(INIT_OK);
        } else if (event == "device_ready") {
            Serial.println("[DisplayDevice] Calling displaySystemMessage(DEVICE_OK)");
            displaySystemMessage(DEVICE_OK);
        }
    } else if (type == "wifi") {
        if (event == "connected" || event == "recovered") {
            Serial.println("[DisplayDevice] Calling displaySystemMessage(WIFI_OK)");
            displaySystemMessage(WIFI_OK);
        }
    } else if (type == "mqtt") {
        if (event == "Connected") {  // Note: capital C in actual event
            Serial.println("[DisplayDevice] Calling displaySystemMessage(MQTT_OK)");
            displaySystemMessage(MQTT_OK);
        }
    }
}
