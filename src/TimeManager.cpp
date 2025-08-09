#include <TimeManager.h>
#include <ConfigurationManager.h>
#include <EventManager.h>
#include <CommandManager.h>
#include <algorithm>


// Constructor is now inline in header


void TimeManager::init() {
    logDebug("Initializing TimeManager", 1);
    
    // Register time commands with CommandManager
    registerCommands();
    
    setInitialized(true);
    logDebug("TimeManager initialized successfully", 1);
}

bool TimeManager::onCommand(const String& command, const std::vector<String>& params) {
    if (command == "date") {
        debug(getFormattedDateTime("%d/%m/%Y %H:%M:%S"), 0);
        return true;
    } else if (command == "time") {
        debug(getFormattedDateTime("%H:%M:%S"), 0);
        return true;
    } else if (command == "ntp") {
        if (update(true)) {
            debug("Time updated", 0);
            debug("Time: " + getFormattedDateTime("%d/%m/%Y %H:%M:%S"), 0);
        } else {
            debug("Failed to update time", 0);
        }
        return true;
    }
    return false; // Command not handled
}

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
        debug("WiFi not connected, cannot update time", 1, false);
        return false;
    }
    // Safely get ConfigurationManager
    Manager* mgr = context ? context->getManager("ConfigurationManager") : nullptr;
    auto* configMgr = mgr ? static_cast<ConfigurationManager*>(mgr) : nullptr;
    
    // Use safe default values if ConfigurationManager is not available
    String ntpServerStr = "pool.ntp.org";
    String timezoneStr = "CET-1CEST,M3.5.0,M10.5.0/3";
    
    if (configMgr) {
        ntpServerStr = configMgr->getPreference("ntp_server", "pool.ntp.org");
        timezoneStr = configMgr->getPreference("timezone", "CET-1CEST,M3.5.0,M10.5.0/3");
    }
    
    debug("Updating time from " + ntpServerStr, 1, false);
    configTime(0, 0, ntpServerStr.c_str());
    setenv("TZ", timezoneStr.c_str(), 1);
    tzset();
    isInitialized = true;
    debug("Time set to: " + getFormattedDateTime("%H:%M:%S"), 1);
    return true;
}

String TimeManager::getFormattedDateTime(const char* format)
{
    if (!isInitialized) {
        if (WiFi.isConnected()) {
            debug("Time not initialized, initializing...", 2, false);
            update();
        } else {
            return String("");
        }
    }
    if (!isInitialized) {
        debug("Failed to initialize time", 1, false);
        return String("");
    }
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo)) {
        // logMessage("Failed to obtain time");
        debug("Failed to obtain time", 2, false);
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
    uint id = nextIntervalId++;
    Interval newInterval = {id, millis(), intervalTime, callback, true};
    intervals.push_back(newInterval);
    return id;
}

uint TimeManager::setIntervalObj(void* obj, std::function<void(void*)> callback, unsigned long intervalTime)
{
    uint id = nextIntervalId++;
    Interval newInterval = {id, millis(), intervalTime, [obj, callback]() { callback(obj); }, true};
    intervals.push_back(newInterval);
    return id;
}

void TimeManager::clearInterval(uint id)
{
    intervals.erase(std::remove_if(intervals.begin(), intervals.end(),
                                    [id](const Interval& i) { return i.id == id; }),
                    intervals.end());
}

void TimeManager::checkTimeouts()
{
    for (auto& timeout : timeouts) {
        if (timeout.active && (millis() - timeout.startTime >= timeout.delay)) {
            debug("Timeout triggered", 2);
            timeout.callback();
            timeout.active = false;
        }
    }
}

uint TimeManager::setTimeout(std::function<void()> callback, unsigned long delay)
{
    uint id = nextTimeoutId++;
    Timeout newTimeout = {id, millis(), delay, callback, true};
    timeouts.push_back(newTimeout);
    return id;
}

uint TimeManager::setTimeoutObj(void* obj, std::function<void(void*)> callback, unsigned long delay)
{
    uint id = nextTimeoutId++;
    Timeout newTimeout = {id, millis(), delay, [obj, callback]() { callback(obj); }, true};
    timeouts.push_back(newTimeout);
    return id;
}

void TimeManager::clearTimeout(uint id)
{
    timeouts.erase(std::remove_if(timeouts.begin(), timeouts.end(),
                                   [id](const Timeout& t) { return t.id == id; }),
                   timeouts.end());
}

void TimeManager::checkSchedulers()
{
    if (!isInitialized) {
        return;
    }

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        debug("Failed to obtain time", 1);
        return;
    }

    int currentHour = timeinfo.tm_hour;
    int currentMinute = timeinfo.tm_min;
    // Convert tm_wday (0=Sunday, 1=Monday, ..., 6=Sat) To 1=Monday, ..., 7=Sunday
    int currentDayOfWeek = (timeinfo.tm_wday == 0) ? 7 : timeinfo.tm_wday;

    char currentDateTime[11];
    strftime(currentDateTime, sizeof(currentDateTime), "%Y-%m-%d", &timeinfo);
    String todayStr = String(currentDateTime);

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
            if (scheduler.lastTriggeredDate != todayStr) {
                debug("Scheduler triggered at " + String(currentHour) + ":" + String(currentMinute), 1);
                #ifdef ESP32
                try {
                    scheduler.callback();
                } catch (const std::exception& e) {
                    debug("Scheduler callback error: " + String(e.what()), 0);
                }
                #else
                scheduler.callback();
                #endif
                scheduler.lastTriggeredDate = todayStr;
            }
        }
    }
}

uint TimeManager::setScheduler(std::function<void()> callback, int hour, int minute, const std::vector<int>& daysOfWeek, const String& startDate,
                               const String& endDate)
{
    uint id = nextSchedulerId++;
    Scheduler newScheduler = {id, hour, minute, daysOfWeek, startDate, endDate, callback, true};
    schedulers.push_back(newScheduler);
    return id;
}

uint TimeManager::setSchedulerObj(void* obj, std::function<void(void*)> callback, int hour, int minute, const std::vector<int>& daysOfWeek,
                                  const String& startDate, const String& endDate)
{
    uint id = nextSchedulerId++;
    Scheduler newScheduler = {id, hour, minute, daysOfWeek, startDate, endDate, [obj, callback]() { callback(obj); }, true};
    schedulers.push_back(newScheduler);
    return id;
}

void TimeManager::clearScheduler(uint id)
{
    schedulers.erase(std::remove_if(schedulers.begin(), schedulers.end(),
                                     [id](const Scheduler& s) { return s.id == id; }),
                     schedulers.end());
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

TimeManager::Program* TimeManager::addProgram(const String& json, std::function<void()> onStart, std::function<void()> onStop)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    Program* program = new Program();

    if (error) {
        debug("Erreur parsing JSON Program", 1);
        delete program;
        return nullptr;
    }

    // Champs obligatoires
    if (!doc["startTime"].is<const char*>()) {
        debug("Champ startTime invalide ou manquant", 1);
        return nullptr;
    }
    if (!doc["duration"].is<uint16_t>()) {
        debug("Champ duration invalide ou manquant", 1);
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

    /*if ((doc["onStart"].isNull() == false) && !doc["onStart"].isNull()) {
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
    }*/

    program->onStart = onStart;
    program->onStop = onStop;

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

void TimeManager::registerCommands()
{
    auto* cmdMgr = static_cast<CommandManager*>(context ? context->getManager("CommandManager") : nullptr);
    if (!cmdMgr) return;

    // Date command
    cmdMgr->registerCommand(Command(
        "time", "date", "Show current date",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            return getFormattedDateTime("%d/%m/%Y");
        }
    ));

    // Time command
    cmdMgr->registerCommand(Command(
        "time", "time", "Show current time",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            return getFormattedDateTime("%H:%M:%S");
        }
    ));

    // DateTime command
    cmdMgr->registerCommand(Command(
        "time", "datetime", "Show current date and time",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            return getFormattedDateTime("%d/%m/%Y %H:%M:%S");
        }
    ));

    // NTP update command
    cmdMgr->registerCommand(Command(
        "time", "ntp", "Update time from NTP server",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            if (update(true)) {
                return "Time updated from NTP: " + getFormattedDateTime("%d/%m/%Y %H:%M:%S");
            } else {
                return "Failed to update time from NTP";
            }
        }
    ));

    // Night status command
    cmdMgr->registerCommand(Command(
        "time", "night", "Check if it's night time",
        CommandSource::Any, false,
        [this](const std::vector<String>& args) -> String {
            return String("Night time: ") + (isNight() ? "Yes" : "No");
        }
    ));


    // Register useful aliases
    cmdMgr->registerAlias("dt", "time:datetime");
    cmdMgr->registerAlias("now", "time:datetime");
}
