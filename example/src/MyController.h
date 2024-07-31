#ifndef MYCONTROLLER_H
#define MYCONTROLLER_H

#include <Arduino.h>
#include <MainController.h>
#include <Configuration.h>
#include <vector>
#include <DHT.h>
#include "config.h"

class MyController: public MainController
{
private:

    MyConfig* myConfig;
    DHT dht;

public:
    MyController(MyConfig& config);

    void init() override;
    void loop() override;

    void processEvent(String type, String action, std::vector<String> params) override;
    void processMQTT(String topic, String value) override;

    void readTemperature();

};

#endif



