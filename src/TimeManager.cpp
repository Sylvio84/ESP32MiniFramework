#include "../include/TimeManager.h"

void TimeManager::init()
{
}


void TimeManager::loop()
{
    checkIntervals();
    checkTimeouts();
    checkSchedulers();
}

void TimeManager::update(bool force)
{
    if (isInitialized && !force)
    {
        return;
    }
    Serial.println("Time not set, setting time...");
    configTime(0, 0, ntpServer);
    setenv("TZ", timezone, 1);
    tzset();
    isInitialized = true;
    Serial.println("Time set to: " + getFormattedDateTime("%H:%M:%S"));
}

String TimeManager::getFormattedDateTime(const char *format)
{
    if (!isInitialized)
    {
        Serial.println("Time not initialized, initializing...");
        update();
    }
    if (!isInitialized)
    {
        Serial.println("Failed to initialize time");
        return String("");
    }
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo))
    {
        // logMessage("Failed to obtain time");
        Serial.println("Failed to obtain time");
        return String("");
    }
    char formattedTime[20];
    strftime(formattedTime, sizeof(formattedTime), format, &timeinfo);
    return String(formattedTime);
}

void TimeManager::checkIntervals()
{
    for (auto &interval : intervals)
    {
        if (interval.active && (millis() - interval.lastTime >= interval.interval))
        {
            interval.callback();
            interval.lastTime = millis();
        }
    }
}

int TimeManager::setInterval(std::function<void()> callback, unsigned long intervalTime)
{
    Interval newInterval = {millis(), intervalTime, callback, true};
    intervals.push_back(newInterval);
    return intervals.size() - 1; // Retourner l'index comme ID d'intervalle
}

int TimeManager::setIntervalObj(void* obj, std::function<void(void*)> callback, unsigned long intervalTime) {
    Interval newInterval = {
        millis(),
        intervalTime,
        [obj, callback]() { callback(obj); },
        true
    };
    intervals.push_back(newInterval);
    return intervals.size() - 1;
}

void TimeManager::clearInterval(int id)
{
    if (id >= 0 && id < intervals.size())
    {
        intervals[id].active = false; // Désactiver l'intervalle
    }
}

void TimeManager::checkTimeouts()
{
    for (auto &timeout : timeouts)
    {
        if (timeout.active && (millis() - timeout.startTime >= timeout.delay))
        {
            timeout.callback();
            timeout.active = false;
        }
    }
}

int TimeManager::setTimeout(std::function<void()> callback, unsigned long delay)
{
    Timeout newTimeout = {millis(), delay, callback, true};
    timeouts.push_back(newTimeout);
    return timeouts.size() - 1; // Retourner l'index comme ID de délai
}

int TimeManager::setTimeoutObj(void* obj, std::function<void(void*)> callback, unsigned long delay) {
    Timeout newTimeout = {
        millis(),
        delay,
        [obj, callback]() { callback(obj); },
        true
    };
    timeouts.push_back(newTimeout);
    return timeouts.size() - 1;
}

void TimeManager::clearTimeout(int id)
{
    if (id >= 0 && id < timeouts.size())
    {
        timeouts[id].active = false; // Désactiver le délai
    }
}


void TimeManager::checkSchedulers()
{
    if (!isInitialized) {
        return;
    }

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return;
    }

    int currentHour = timeinfo.tm_hour;
    int currentMinute = timeinfo.tm_min;
    int currentDayOfWeek = timeinfo.tm_wday;

    for (auto &scheduler : schedulers)
    {
        if (scheduler.active &&
            scheduler.hour == currentHour &&
            scheduler.minute == currentMinute &&
            std::find(scheduler.daysOfWeek.begin(), scheduler.daysOfWeek.end(), currentDayOfWeek) != scheduler.daysOfWeek.end())
        {
            scheduler.callback();
        }
    }
}

int TimeManager::setScheduler(std::function<void()> callback, int hour, int minute, const std::vector<int>& daysOfWeek)
{
    Scheduler newScheduler = {hour, minute, daysOfWeek, callback, true};
    schedulers.push_back(newScheduler);
    return schedulers.size() - 1; // Retourner l'index comme ID de planification
}

int TimeManager::setSchedulerObj(void* obj, std::function<void(void*)> callback, int hour, int minute, const std::vector<int>& daysOfWeek)
{
    Scheduler newScheduler = {hour, minute, daysOfWeek, [obj, callback]() { callback(obj); }, true};
    schedulers.push_back(newScheduler);
    return schedulers.size() - 1; // Retourner l'index comme ID de planification
}

void TimeManager::clearScheduler(int id)
{
    if (id >= 0 && id < schedulers.size())
    {
        schedulers[id].active = false; // Désactiver la planification
    }
}