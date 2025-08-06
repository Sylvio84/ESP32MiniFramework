#include <TimeManager.h>

EventManager* TimeManager::eventManager = nullptr;

TimeManager::TimeManager(Configuration& config, EventManager& eventMgr) : config(config)
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
    // Convert tm_wday (0=Sunday, 1=Monday, ..., 6=Sat) To 1=Monday, ..., 7=Sunday
    int currentDayOfWeek = (timeinfo.tm_wday == 0) ? 7 : timeinfo.tm_wday;

    for (auto& scheduler : schedulers) {
        if (!scheduler.active)
            continue;

        // Vérifie les jours de la semaine
        if (!scheduler.daysOfWeek.empty() &&
            std::find(scheduler.daysOfWeek.begin(), scheduler.daysOfWeek.end(), currentDayOfWeek) == scheduler.daysOfWeek.end()) {
            continue;
        }

        // Vérifie la date dans la plage
        if (!scheduler.startDate.isEmpty() && !scheduler.endDate.isEmpty() && !isDateInRange(timeinfo, scheduler.startDate, scheduler.endDate)) {
            continue;
        }

        // Vérifie heure/minute
        if (scheduler.hour == currentHour && scheduler.minute == currentMinute) {
            scheduler.callback();
        }
    }
}

uint TimeManager::setScheduler(std::function<void()> callback, int hour, int minute, const std::vector<int>& daysOfWeek, const String& startDate,
                               const String& endDate)
{
    Scheduler newScheduler = {hour, minute, daysOfWeek, startDate, endDate, callback, true};
    schedulers.push_back(newScheduler);
    return schedulers.size() - 1;
}

uint TimeManager::setSchedulerObj(void* obj, std::function<void(void*)> callback, int hour, int minute, const std::vector<int>& daysOfWeek,
                                  const String& startDate, const String& endDate)
{
    Scheduler newScheduler = {hour, minute, daysOfWeek, startDate, endDate, [obj, callback]() { callback(obj); }, true};
    schedulers.push_back(newScheduler);
    return schedulers.size() - 1;
}

void TimeManager::clearScheduler(uint id)
{
    if (id >= 0 && id < schedulers.size()) {
        schedulers.erase(schedulers.begin() + id);
    }
}

std::tm TimeManager::timeToDate(const std::string& time, const std::tm& now)
{
    std::tm date = now;
    int hours = std::stoi(time.substr(0, 2));
    int minutes = std::stoi(time.substr(3, 2));
    date.tm_hour = hours;
    date.tm_min = minutes;
    date.tm_sec = 0;
    return date;
}

bool TimeManager::isNight()
{
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

bool TimeManager::isDateInRange(const std::tm& now, const String& startDate, const String& endDate)
{
    int dayNow = now.tm_mday;
    int monthNow = now.tm_mon + 1;

    int startDay = startDate.substring(0, 2).toInt();
    int startMonth = startDate.substring(3, 5).toInt();

    int endDay = endDate.substring(0, 2).toInt();
    int endMonth = endDate.substring(3, 5).toInt();

    if ((monthNow > startMonth || (monthNow == startMonth && dayNow >= startDay)) && (monthNow < endMonth || (monthNow == endMonth && dayNow <= endDay))) {
        return true;
    }
    // Si la période chevauche la fin d'année
    if ((startMonth > endMonth) &&
        ((monthNow > startMonth || (monthNow == startMonth && dayNow >= startDay)) || (monthNow < endMonth || (monthNow == endMonth && dayNow <= endDay)))) {
        return true;
    }
    return false;
}

void TimeManager::initProgram(Program& program)
{
    int hour = program.startTime.substring(0, 2).toInt();
    int minute = program.startTime.substring(3, 5).toInt();

    uint startId = setScheduler(program.onStart, hour, minute, program.daysOfWeek, program.startDate, program.endDate);

    uint stopId = 0;
    if (program.duration > 0 && program.onStop) {
        int endHour = hour;
        int endMinute = minute + program.duration;
        endHour += endMinute / 60;
        endMinute %= 60;
        endHour %= 24;

        stopId = setScheduler(program.onStop, endHour, endMinute, program.daysOfWeek, program.startDate, program.endDate);
    }

    program.startSchedulerId = startId;
    program.stopSchedulerId = stopId;

    programs.push_back(program);
}

TimeManager::Program* TimeManager::addProgram(const String& json, std::function<void(String)> onStartCbBuilder, std::function<void(String)> onStopCbBuilder)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    Program* program = new Program();

    if (error) {
        eventManager->debug("Erreur parsing JSON Program", 1);
        delete program;
        return nullptr;
    }

    // Champs obligatoires
    if (!doc["startTime"].is<const char*>()) {
        eventManager->debug("Champ startTime invalide ou manquant", 1);
        return nullptr;
    }
    if (!doc["duration"].is<uint16_t>()) {
        eventManager->debug("Champ duration invalide ou manquant", 1);
        return nullptr;
    }


    String dump;
    serializeJson(doc, dump);
    
    program->startTime = doc["startTime"].as<String>();
    program->duration = doc["duration"].as<uint16_t>();

    // Valeurs par défaut
    program->startDate = doc["startDate"] | "";
    program->endDate = doc["endDate"] | "";
    program->active = doc["active"] | true;

    if ((doc["days"].isNull() == false) && doc["days"].is<JsonArray>()) {
        for (JsonVariant v : doc["days"].as<JsonArray>()) {
            program->daysOfWeek.push_back(v.as<int>());
        }
    }

    if ((doc["onStart"].isNull() == false) && !doc["onStart"].isNull()) {
        String id = doc["onStart"].as<String>();
        program->onStart = [id, onStartCbBuilder]() {
            onStartCbBuilder(id);
        };
    }

    if ((doc["onStop"].isNull() == false) && !doc["onStop"].isNull()) {
        String id = doc["onStop"].as<String>();
        program->onStop = [id, onStopCbBuilder]() {
            onStopCbBuilder(id);
        };
    }

    initProgram(*program);

    return program;
}

String TimeManager::exportProgramToJson(const Program& program)
{
    JsonDocument doc;

    doc["startTime"] = program.startTime;
    doc["duration"] = program.duration;

    if (!program.startDate.isEmpty())
        doc["startDate"] = program.startDate;
    if (!program.endDate.isEmpty())
        doc["endDate"] = program.endDate;

    if (!program.daysOfWeek.empty()) {
        JsonArray days = doc["days"].to<JsonArray>();
        for (int day : program.daysOfWeek) {
            days.add(day);
        }
    }

    doc["active"] = program.active;

    String output;
    serializeJson(doc, output);
    return output;
}
