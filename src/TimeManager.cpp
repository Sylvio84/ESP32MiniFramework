#include "../include/TimeManager.h"

EventManager* TimeManager::eventManager = nullptr;

TimeManager::TimeManager(Configuration& config, EventManager& eventMgr): config(config)
{
    this->eventManager = &eventMgr;
}

void TimeManager::init() {}

void TimeManager::loop()
{
    checkIntervals();
    checkTimeouts();
    checkSchedulers();
}

bool TimeManager::update(bool force)
{
    if (isInitialized && !force) {
        return true;
    }
    if (!WiFi.isConnected()) {
        eventManager->debug("WiFi not connected, cannot update time", 1, false);
        return false;
    }
    eventManager->debug("Updating time from " + String(config.NTP_SERVER), 1, false);
    configTime(0, 0, config.NTP_SERVER);
    setenv("TZ", config.TIMEZONE, 1);
    tzset();
    isInitialized = true;
    eventManager->debug("Time set to: " + getFormattedDateTime("%H:%M:%S"), 1);
    return true;
}

String TimeManager::getFormattedDateTime(const char* format)
{
    if (!isInitialized) {
        if (WiFi.isConnected()) {
            eventManager->debug("Time not initialized, initializing...", 2, false);
            update();
        } else {
            return String("");
        }
    }
    if (!isInitialized) {
        eventManager->debug("Failed to initialize time", 1, false);
        return String("");
    }
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo)) {
        // logMessage("Failed to obtain time");
        eventManager->debug("Failed to obtain time", 2, false);
        return String("");
    }
    char formattedTime[20];
    strftime(formattedTime, sizeof(formattedTime), format, &timeinfo);
    return String(formattedTime);
}

void TimeManager::checkIntervals()
{
    for (auto& interval : intervals) {
        if (interval.active && (millis() - interval.lastTime >= interval.interval)) {
            interval.callback();
            interval.lastTime = millis();
        }
    }
}

uint TimeManager::setInterval(std::function<void()> callback, unsigned long intervalTime)
{
    Interval newInterval = {millis(), intervalTime, callback, true};
    intervals.push_back(newInterval);
    return intervals.size() - 1;
}

uint TimeManager::setIntervalObj(void* obj, std::function<void(void*)> callback, unsigned long intervalTime)
{
    Interval newInterval = {millis(), intervalTime, [obj, callback]() { callback(obj); }, true};
    intervals.push_back(newInterval);
    return intervals.size() - 1;
}

void TimeManager::clearInterval(uint id)
{
    if (id >= 0 && id < intervals.size()) {
        intervals[id].active = false;
    }
}

void TimeManager::checkTimeouts()
{
    for (auto& timeout : timeouts) {
        if (timeout.active && (millis() - timeout.startTime >= timeout.delay)) {
            eventManager->debug("Timeout triggered", 2);
            timeout.callback();
            timeout.active = false;
        }
    }
}

uint TimeManager::setTimeout(std::function<void()> callback, unsigned long delay)
{
    Timeout newTimeout = {millis(), delay, callback, true};
    timeouts.push_back(newTimeout);
    return timeouts.size() - 1;  // Retourner l'index comme ID de délai
}

uint TimeManager::setTimeoutObj(void* obj, std::function<void(void*)> callback, unsigned long delay)
{
    Timeout newTimeout = {millis(), delay, [obj, callback]() { callback(obj); }, true};
    timeouts.push_back(newTimeout);
    return timeouts.size() - 1;
}

void TimeManager::clearTimeout(uint id)
{
    if (id >= 0 && id < timeouts.size()) {
        timeouts.erase(timeouts.begin() + id);
    }
}

void TimeManager::checkSchedulers()
{
    if (!isInitialized) {
        return;
    }

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        eventManager->debug("Failed to obtain time", 1);
        return;
    }

    int currentHour = timeinfo.tm_hour;
    int currentMinute = timeinfo.tm_min;
    int currentDayOfWeek = timeinfo.tm_wday;

    for (auto& scheduler : schedulers) {
        if (scheduler.active && scheduler.hour == currentHour && scheduler.minute == currentMinute &&
            std::find(scheduler.daysOfWeek.begin(), scheduler.daysOfWeek.end(), currentDayOfWeek) != scheduler.daysOfWeek.end()) {
            scheduler.callback();
        }
    }
}

uint TimeManager::setScheduler(std::function<void()> callback, int hour, int minute, const std::vector<int>& daysOfWeek)
{
    Scheduler newScheduler = {hour, minute, daysOfWeek, callback, true};
    schedulers.push_back(newScheduler);
    return schedulers.size() - 1;
}

uint TimeManager::setSchedulerObj(void* obj, std::function<void(void*)> callback, int hour, int minute, const std::vector<int>& daysOfWeek)
{
    Scheduler newScheduler = {hour, minute, daysOfWeek, [obj, callback]() { callback(obj); }, true};
    schedulers.push_back(newScheduler);
    return schedulers.size() - 1;
}

void TimeManager::clearScheduler(uint id)
{
    if (id >= 0 && id < schedulers.size()) {
        schedulers.erase(schedulers.begin() + id);
    }
}

std::tm TimeManager::timeToDate(const std::string& time, const std::tm& now) {
    std::tm date = now;
    int hours = std::stoi(time.substr(0, 2));
    int minutes = std::stoi(time.substr(3, 2));
    date.tm_hour = hours;
    date.tm_min = minutes;
    date.tm_sec = 0;
    return date;
}

bool TimeManager::isNight() {
    if (!update()) {  // Ensure time is up-to-date
        return false;
    }

    std::time_t t = std::time(nullptr);
    std::tm now = *std::localtime(&t);
    int currentMonth = now.tm_mon + 1;

    SunTime currentSunTime = sunTimes[currentMonth];
    std::tm sunriseTime = timeToDate(currentSunTime.sunrise, now);
    std::tm sunsetTime = timeToDate(currentSunTime.sunset, now);

    std::time_t nowTime = std::mktime(&now);
    std::time_t sunrise = std::mktime(&sunriseTime);
    std::time_t sunset = std::mktime(&sunsetTime);

    return nowTime < sunrise || nowTime >= sunset;
}
