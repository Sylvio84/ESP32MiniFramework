#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <Arduino.h>
#include <Configuration.h>
#include <vector>
#include <functional>

class TimeManager
{
protected:
    bool isInitialized = false;

    const char *ntpServer = "pool.ntp.org";
    const char *timezone = "CET-1CEST,M3.5.0,M10.5.0/3";

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
    TimeManager(Configuration config)
    {
        timezone = config.TIMEZONE;
        ntpServer = config.NTP_SERVER;
    }

    virtual void init();
    virtual void loop();
    
    void update(bool force = false);

    String getFormattedDateTime(const char *format);


    int setTimeout(std::function<void()> callback, unsigned long delay);
    int setTimeoutObj(void* obj, std::function<void(void*)> callback, unsigned long delay);
    void clearTimeout(int id);

    int setInterval(std::function<void()> callback, unsigned long intervalTime);
    int setIntervalObj(void* obj, std::function<void(void*)> callback, unsigned long intervalTime);
    void clearInterval(int id);
    
    // @todo: to test
    int setScheduler(std::function<void()> callback, int hour, int minute, const std::vector<int>& daysOfWeek);
    int setSchedulerObj(void* obj, std::function<void(void*)> callback, int hour, int minute, const std::vector<int>& daysOfWeek);
    void clearScheduler(int id);
};

#endif
