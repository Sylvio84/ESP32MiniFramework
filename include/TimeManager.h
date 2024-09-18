#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <Arduino.h>
#include <Configuration.h>
#include <vector>
#include <functional>
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <EventManager.h>

class TimeManager
{
protected:
    Configuration& config;
    static EventManager* eventManager;  // Pointeur vers EventManager

    struct Timeout {
        unsigned long startTime;
        unsigned long delay;
        std::function<void()> callback;
        bool active;
    };

    struct Interval {
        unsigned long lastTime;
        unsigned long interval;
        std::function<void()> callback;
        bool active;
    };

    struct Scheduler {
        int hour;
        int minute;
        std::vector<int> daysOfWeek; // 0 (Sunday) to 6 (Saturday)
        std::function<void()> callback;
        bool active;
    };

    std::vector<Timeout> timeouts;
    std::vector<Interval> intervals;
    std::vector<Scheduler> schedulers;


    void checkIntervals();
    void checkTimeouts();
    void checkSchedulers(); // @todo: to test

public:

    bool isInitialized = false;


    TimeManager(Configuration& config, EventManager& eventMgr) : config(config)
    {
        this->eventManager = &eventMgr;
    }

    virtual void init();
    virtual void loop();
    
    bool update(bool force = false);

    String getFormattedDateTime(const char *format);


    uint setTimeout(std::function<void()> callback, unsigned long delay);
    uint setTimeoutObj(void* obj, std::function<void(void*)> callback, unsigned long delay);
    void clearTimeout(uint id);

    uint setInterval(std::function<void()> callback, unsigned long intervalTime);
    uint setIntervalObj(void* obj, std::function<void(void*)> callback, unsigned long intervalTime);
    void clearInterval(uint id);
    
    // @todo: to test
    uint setScheduler(std::function<void()> callback, int hour, int minute, const std::vector<int>& daysOfWeek);
    uint setSchedulerObj(void* obj, std::function<void(void*)> callback, int hour, int minute, const std::vector<int>& daysOfWeek);
    void clearScheduler(uint id);
};

#endif