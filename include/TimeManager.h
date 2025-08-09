#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <Arduino.h>
#include <FrameworkContext.h>
#include <Manager.h>
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

// Forward declarations
class CommandManager;


/**
 * @brief TimeManager - Handles time-related operations and scheduling
 * 
 * Provides:
 * - Date and time formatting
 * - Timeout and interval scheduling
 * - Scheduler for recurring tasks
 * - NTP time synchronization
 * - Sun time calculations
 * @note This manager is designed to be used with a WiFi connection for NTP updates.
 * @note Ensure to call `update()` periodically to keep the time accurate.
 * @note The manager supports scheduling tasks based on time and date.
 * @note The manager can be extended to support more complex scheduling and time-based operations.
 */

class TimeManager : public Manager
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

    // Constructor with dependency injection
    TimeManager(FrameworkContext& ctx) : Manager(ctx) {}
    
    // Implement Manager interface
    void init() override;
    void loop() override;
    String getName() const override { return "TimeManager"; }
    bool onCommand(const String& command, const std::vector<String>& params) override;
    
    void registerCommands();

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
    TimeManager::Program* addProgram(const String& json, std::function<void()> onStart, std::function<void()> onStop);
    static String exportProgramToJson(const Program& program);

    bool isNight();

  protected:
    FrameworkContext* context;

    struct Timeout
    {
        uint id;
        unsigned long startTime;
        unsigned long delay;
        std::function<void()> callback;
        bool active;
    };

    struct Interval
    {
        uint id;
        unsigned long lastTime;
        unsigned long interval;
        std::function<void()> callback;
        bool active;
    };

    struct Scheduler
    {
        uint id;
        int hour;
        int minute;
        std::vector<int> daysOfWeek;  // 0 (Sunday) to 6 (Saturday)
        String startDate;             // Format "DD/MM"
        String endDate;               // Format "DD/MM"
        std::function<void()> callback;
        bool active;
        String lastTriggeredDate;
    };

    std::vector<Timeout> timeouts;
    std::vector<Interval> intervals;
    std::vector<Scheduler> schedulers;
    std::vector<Program> programs;
    
    uint nextTimeoutId = 1;
    uint nextIntervalId = 1;
    uint nextSchedulerId = 1;

    void checkIntervals();
    void checkTimeouts();
    void checkSchedulers();  // @todo: to test

    bool isDateInRange(const std::tm& now, const String& startDate, const String& endDate);
};

#endif
