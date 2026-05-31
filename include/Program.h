#ifndef PROGRAM_H
#define PROGRAM_H

#include <Arduino.h>
#include <vector>
#include <functional>
#include <ctime>
#include <Tools.h>

class Program
{
private:
    std::function<void()> onStart;
    std::function<void()> onStop;

    bool active = true;

    uint startSchedulerId = -1;
    uint stopSchedulerId = -1;

public:
    String startDate;             // Format "DD/MM"
    String endDate;               // Format "DD/MM"
    String startTime;             // Format "HH:MM:SS"
    uint16_t duration;            // en secondes
    std::vector<int> daysOfWeek;  // 0=Dimanche, 1=Lundi, ..., 6=Samedi

    Program();
    Program(const String& startDate, const String& endDate, const String& startTime, uint16_t duration);
    ~Program();

    // Getters
    String getStartDate() const;
    String getEndDate() const;
    String getStartTime() const;
    uint16_t getDuration() const;
    std::vector<int> getDaysOfWeek() const;
    bool isActive() const;
    uint getStartSchedulerId() const;
    uint getStopSchedulerId() const;

    // Setters
    void setStartDate(const String& startDate);
    void setEndDate(const String& endDate);
    void setStartTime(const String& startTime);
    void setDuration(uint16_t duration);
    void setDaysOfWeek(const std::vector<int>& daysOfWeek);
    void setActive(bool active);
    void setStartSchedulerId(uint id);
    void setStopSchedulerId(uint id);

    // Callback management
    void setOnStartCallback(std::function<void()> callback);
    void setOnStopCallback(std::function<void()> callback);
    void executeOnStart();
    void executeOnStop();

    // Program state management
    bool isRunning() const;
    void start();
    void stop();
    void reset();

    // Validation
    bool isValid() const;
};

#endif