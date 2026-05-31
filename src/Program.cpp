#include "Program.h"

Program::Program()
    : duration(0), active(true), startSchedulerId(-1), stopSchedulerId(-1)
{
}

Program::Program(const String& startDate, const String& endDate, const String& startTime, uint16_t duration)
    : startDate(startDate), endDate(endDate), startTime(startTime), duration(duration),
      active(true), startSchedulerId(-1), stopSchedulerId(-1)
{
}

Program::~Program()
{
}

// Getters
String Program::getStartDate() const
{
    return startDate;
}

String Program::getEndDate() const
{
    return endDate;
}

String Program::getStartTime() const
{
    return startTime;
}

uint16_t Program::getDuration() const
{
    return duration;
}

std::vector<int> Program::getDaysOfWeek() const
{
    return daysOfWeek;
}

bool Program::isActive() const
{
    return active;
}

uint Program::getStartSchedulerId() const
{
    return startSchedulerId;
}

uint Program::getStopSchedulerId() const
{
    return stopSchedulerId;
}

// Setters
void Program::setStartDate(const String& startDate)
{
    this->startDate = startDate;
}

void Program::setEndDate(const String& endDate)
{
    this->endDate = endDate;
}

void Program::setStartTime(const String& startTime)
{
    this->startTime = startTime;
}

void Program::setDuration(uint16_t duration)
{
    this->duration = duration;
}

void Program::setDaysOfWeek(const std::vector<int>& daysOfWeek)
{
    this->daysOfWeek = daysOfWeek;
}

void Program::setActive(bool active)
{
    this->active = active;
}

void Program::setStartSchedulerId(uint id)
{
    this->startSchedulerId = id;
}

void Program::setStopSchedulerId(uint id)
{
    this->stopSchedulerId = id;
}

// Callback management
void Program::setOnStartCallback(std::function<void()> callback)
{
    this->onStart = callback;
}

void Program::setOnStopCallback(std::function<void()> callback)
{
    this->onStop = callback;
}

void Program::executeOnStart()
{
    if (onStart)
    {
        onStart();
    }
}

void Program::executeOnStop()
{
    if (onStop)
    {
        onStop();
    }
}

// Program state management
bool Program::isRunning() const
{
    // Si on a des IDs valides de scheduler (facultatif selon ta logique)
    /*if (startSchedulerId != std::numeric_limits<uint>::max() &&
        stopSchedulerId  != std::numeric_limits<uint>::max()) {
        return true;
    }*/

    std::time_t t = std::time(nullptr);
    std::tm local = *std::localtime(&t);

    int hour=0, minute=0, second=0;
    if (std::sscanf(startTime.c_str(), "%2d:%2d:%2d", &hour, &minute, &second) != 3)
        return false; // format invalide

    std::tm startTm = local;
    startTm.tm_hour = hour;
    startTm.tm_min  = minute;
    startTm.tm_sec  = second;

    std::time_t startEpoch = std::mktime(&startTm);
    if (startEpoch == -1) return false;

    std::time_t endEpoch = startEpoch + duration;
    if (t < startEpoch || t > endEpoch) return false;

    // Vérifie le jour de la semaine
    if (!daysOfWeek.empty()) {
        int wday = local.tm_wday; // 0=Dimanche
        bool match = false;
        for (int d : daysOfWeek) {
            if (d == wday) { match = true; break; }
        }
        if (!match) return false;
    }

    // Vérifie la plage de dates DD/MM
    if (!startDate.isEmpty() || !endDate.isEmpty()) {
        if (!isDateInRange(local, startDate, endDate)) return false;
    }

    return true;
}

void Program::start()
{
    active = true;
    executeOnStart();
}

void Program::stop()
{
    executeOnStop();
}

void Program::reset()
{
    startSchedulerId = -1;
    stopSchedulerId = -1;
    active = false;
}

// Validation
bool Program::isValid() const
{
    return !startDate.isEmpty() && 
           !endDate.isEmpty() && 
           !startTime.isEmpty() && 
           duration > 0;
}