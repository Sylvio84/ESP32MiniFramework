#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <Arduino.h>
#include <Configuration.h>
#include <ctime>
#include <functional>
#include <map>
#include <vector>
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <ArduinoJson.h>
#include <EventManager.h>

class TimeManager
{

  public:
    struct SunTime
    {
        std::string sunrise;
        std::string sunset;
    };
    std::map<int, SunTime> sunTimes = {
        {1, {"08:10", "17:30"}},   // January
        {2, {"07:40", "18:10"}},   // February
        {3, {"07:00", "18:45"}},   // March
        {4, {"06:45", "20:15"}},   // April
        {5, {"06:10", "20:50"}},   // May
        {6, {"05:50", "21:15"}},   // June
        {7, {"06:00", "21:15"}},   // July
        {8, {"06:30", "20:45"}},   // August
        {9, {"07:00", "19:55"}},   // September
        {10, {"07:30", "18:55"}},  // October
        {11, {"08:00", "17:20"}},  // November
        {12, {"08:20", "17:05"}}   // December
    };

    std::tm timeToDate(const std::string& time, const std::tm& now);

    struct Program
    {
        String startDate;             // Format "DD/MM"
        String endDate;               // Format "DD/MM"
        String startTime;             // Format "HH:MM"
        uint16_t duration;            // en minutes
        std::vector<int> daysOfWeek;  // 0=Dimanche, 1=Lundi, ..., 6=Samedi

        std::function<void()> onStart;
        std::function<void()> onStop;

        bool active = true;

        uint startSchedulerId = -1;
        uint stopSchedulerId = -1;
    };

    bool isInitialized = false;

    TimeManager(Configuration& config, EventManager& eventMgr);

    virtual void init();
    virtual void loop();

    bool update(bool force = false);

    String getFormattedDateTime(const char* format);

    uint setTimeout(std::function<void()> callback, unsigned long delay);
    uint setTimeoutObj(void* obj, std::function<void(void*)> callback, unsigned long delay);
    void clearTimeout(uint id);

    uint setInterval(std::function<void()> callback, unsigned long intervalTime);
    uint setIntervalObj(void* obj, std::function<void(void*)> callback, unsigned long intervalTime);
    void clearInterval(uint id);

    // @todo: to test
    uint setScheduler(std::function<void()> callback, int hour, int minute, const std::vector<int>& daysOfWeek, const String& startDate = "",
                      const String& endDate = "");
    uint setSchedulerObj(void* obj, std::function<void(void*)> callback, int hour, int minute, const std::vector<int>& daysOfWeek, const String& startDate = "",
                         const String& endDate = "");
    void clearScheduler(uint id);

    void initProgram(Program& program);
    TimeManager::Program addProgram(const String& json, std::function<void(String)> onStartCbBuilder, std::function<void(String)> onStopCbBuilder);
    static String exportProgramToJson(const Program& program);

    bool isNight();

  protected:
    Configuration& config;
    static EventManager* eventManager;  // Pointeur vers EventManager

    struct Timeout
    {
        unsigned long startTime;
        unsigned long delay;
        std::function<void()> callback;
        bool active;
    };

    struct Interval
    {
        unsigned long lastTime;
        unsigned long interval;
        std::function<void()> callback;
        bool active;
    };

    struct Scheduler
    {
        int hour;
        int minute;
        std::vector<int> daysOfWeek;  // 0 (Sunday) to 6 (Saturday)
        String startDate;             // Format "DD/MM"
        String endDate;               // Format "DD/MM"
        std::function<void()> callback;
        bool active;
    };

    std::vector<Timeout> timeouts;
    std::vector<Interval> intervals;
    std::vector<Scheduler> schedulers;
    std::vector<Program> programs;

    void checkIntervals();
    void checkTimeouts();
    void checkSchedulers();  // @todo: to test

    bool isDateInRange(const std::tm& now, const String& startDate, const String& endDate);
};

#endif
