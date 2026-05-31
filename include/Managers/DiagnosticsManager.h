#ifndef DIAGNOSTICSMANAGER_H
#define DIAGNOSTICSMANAGER_H

#include "Manager.h"
#include <Arduino.h>

#ifdef ESP32
#include <Preferences.h>
#include <esp_system.h>
#endif

/**
 * @brief DiagnosticsManager - Black box / health monitoring for long-term stability
 *
 * Answers the question "what happened?" after the device crashes or vanishes from the
 * network after days/weeks of operation. Three complementary channels:
 *
 * ### 1. Core Dump (flash)
 * The Arduino-ESP32 prebuilt SDK enables core-dump-to-flash (ELF) by default; the only
 * requirement is a `coredump` partition (present in partitions_1.5mb_ota.csv). On any
 * panic / abort / watchdog reset, a crash image is written to flash. At the next boot
 * this manager detects it (esp_core_dump_image_check), extracts a summary (faulting task
 * + PC) and archives it. Full symbolized backtrace is obtained host-side via `esp-coredump`.
 *
 * ### 2. Persistent black box (NVS + LittleFS)
 * - NVS namespace "diag": bootCount, lastResetReason, lastUptimeSec, minFreeHeapEver.
 * - LittleFS /diag/events.log (rotating): boot/reset, wifi lost/reconnect, mqtt down,
 *   low heap, OTA, reboot — captured through onEvent() and sampling.
 *
 * ### 3. MQTT telemetry
 * Periodic JSON on `<hostname>/telemetry`: heap (free/min/maxAlloc/frag), uptime, RSSI,
 * wifi reconnects, mqtt disconnects, reset reason, boot count. Lets you watch the device
 * degrade remotely (a slowly falling freeHeap = memory leak).
 *
 * ### Commands
 * - `diag:dump`     - reset reason, bootCount, previous uptime, minHeap, recent events
 * - `diag:coredump` - summary of the last core dump
 * - `diag:clear`    - wipe black box / counters / core dump
 */
class DiagnosticsManager : public Manager
{
  public:
    explicit DiagnosticsManager(FrameworkContext& ctx) : Manager(ctx) {}

    void init() override;
    void loop() override;
    bool onEvent(const String& type, const String& event, const std::vector<String>& params) override;
    String getName() const override { return "DiagnosticsManager"; }

  private:
    // === Tuning ===
    static constexpr unsigned long TELEMETRY_INTERVAL_MS = 60000;   // MQTT telemetry cadence
    static constexpr unsigned long HEARTBEAT_NVS_INTERVAL_MS = 300000; // persist uptime/heap (flash wear)
    static constexpr uint32_t LOW_HEAP_THRESHOLD = 20000;          // bytes
    static constexpr size_t EVENTS_LOG_MAX = 6144;                 // bytes before rotation
    static constexpr const char* EVENTS_PATH = "/diag/events.log";
    static constexpr const char* NVS_NS = "diag";

    // === Helpers ===
    void registerCommands();
    void publishTelemetry();
    String buildTelemetryJson();
    String buildDumpString();
    String buildCoreDumpString();
    void appendEvent(const String& event);
    String readEventsLog();
    void persistHeartbeat();
    static String resetReasonString(int reason);

    String getHostname();

    // === State ===
    unsigned long lastTelemetry = 0;
    unsigned long lastHeartbeatPersist = 0;
    unsigned long bootCount = 0;
    String lastResetReason = "UNKNOWN";
    int resetReasonCode = 0;
    unsigned long lastUptimeSec = 0;       // uptime of the PREVIOUS run (from NVS)
    uint32_t minFreeHeapEver = 0xFFFFFFFF; // across runs
    bool lowHeapNotified = false;
    bool coreDumpPresent = false;
    String coreDumpSummary = "";

    bool lastMqttConnected = false;
    unsigned long mqttDisconnects = 0;
};

#endif // DIAGNOSTICSMANAGER_H
