#include "Managers/DiagnosticsManager.h"
#include "Managers/CommandManager.h"
#include "Managers/ConfigurationManager.h"
#include "Managers/MQTTManager.h"
#include "Managers/WiFiManager.h"
#include <Command.h>
#include <LittleFS.h>

#ifdef ESP32
#include <esp_system.h>
#if __has_include(<esp_core_dump.h>)
#include <esp_core_dump.h>
#define DIAG_HAS_COREDUMP 1
#endif
#endif

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void DiagnosticsManager::init()
{
    debug("DiagnosticsManager init...", 1);

    // Ensure the filesystem is mounted (idempotent; SystemManager also begins it).
#ifdef ESP32
    if (!LittleFS.begin(true)) {
#else
    if (!LittleFS.begin()) {
#endif
        debug("DiagnosticsManager: LittleFS mount failed, persistent log disabled", 0);
    } else {
        LittleFS.mkdir("/diag");
    }

#ifdef ESP32
    // --- Reset reason of THIS boot ---
    resetReasonCode = (int)esp_reset_reason();
    lastResetReason = resetReasonString(resetReasonCode);

    // --- Load the black box from NVS (previous run) ---
    Preferences prefs;
    if (prefs.begin(NVS_NS, false)) {
        bootCount = prefs.getULong("bootCount", 0) + 1;
        lastUptimeSec = prefs.getULong("lastUptime", 0);
        minFreeHeapEver = prefs.getULong("minHeap", 0xFFFFFFFF);
        prefs.putULong("bootCount", bootCount);
        prefs.putString("lastReset", lastResetReason);
        prefs.end();
    }

    // --- Core dump detection (crash backtrace captured in flash) ---
#ifdef DIAG_HAS_COREDUMP
    if (esp_core_dump_image_check() == ESP_OK) {
        coreDumpPresent = true;
        coreDumpSummary = buildCoreDumpString();
        debug("DiagnosticsManager: CORE DUMP present from last crash -> " + coreDumpSummary, 0);
    }
#endif
#endif

    // --- Post-mortem summary (logged + archived) ---
    String postMortem = "BOOT #" + String(bootCount) + " reset=" + lastResetReason +
                        " prevUptime=" + String(lastUptimeSec) + "s minHeapEver=" + String(minFreeHeapEver) +
                        (coreDumpPresent ? " [CORE DUMP]" : "");
    debug(postMortem, 0);
    appendEvent(postMortem);

    registerCommands();
    setInitialized(true);
}

void DiagnosticsManager::loop()
{
    unsigned long now = millis();

    // Track the lowest free heap ever seen (cheap, every loop).
    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < minFreeHeapEver) {
        minFreeHeapEver = freeHeap;
    }

    // Low-heap early warning (fires once until heap recovers) — catches slow leaks.
    if (freeHeap < LOW_HEAP_THRESHOLD) {
        if (!lowHeapNotified) {
            lowHeapNotified = true;
            String msg = "LOW HEAP: " + String(freeHeap) + " bytes free";
            debug(msg, 0);
            appendEvent(msg);
            publishEvent("sys", "low_memory", {String(freeHeap)});
        }
    } else if (freeHeap > LOW_HEAP_THRESHOLD + (LOW_HEAP_THRESHOLD / 2)) {
        lowHeapNotified = false;  // hysteresis to avoid event flapping
    }

    // Sample MQTT connection transitions (connected -> disconnected).
    auto* mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
    bool mqttNow = mqttMgr && mqttMgr->isConnected();
    if (lastMqttConnected && !mqttNow) {
        mqttDisconnects++;
        appendEvent("mqtt disconnected (#" + String(mqttDisconnects) + ")");
    }
    lastMqttConnected = mqttNow;

    // Periodic MQTT telemetry.
    if (now - lastTelemetry >= TELEMETRY_INTERVAL_MS) {
        lastTelemetry = now;
        publishTelemetry();
    }

    // Persist heartbeat (uptime + minHeap) to NVS, spaced out to spare the flash.
    if (now - lastHeartbeatPersist >= HEARTBEAT_NVS_INTERVAL_MS) {
        lastHeartbeatPersist = now;
        persistHeartbeat();
    }
}

bool DiagnosticsManager::onEvent(const String& type, const String& event, const std::vector<String>& params)
{
    // Observe key events for the black box. Never consume them (return false).
    if (type == "wifi") {
        if (event == "lost") {
            appendEvent("wifi lost");
        } else if (event == "reconnect_attempt") {
            appendEvent("wifi reconnect attempt #" + (params.empty() ? String("?") : params[0]));
        } else if (event == "recovered" || event == "connected") {
            appendEvent("wifi up" + (params.empty() ? String("") : (" (" + params[0] + ")")));
        } else if (event == "failed") {
            appendEvent("wifi connection failed");
        }
    } else if (type == "sys" && (event == "ota" || event == "reboot")) {
        appendEvent("sys " + event);
    }
    return false;
}

// ---------------------------------------------------------------------------
// Telemetry
// ---------------------------------------------------------------------------

void DiagnosticsManager::publishTelemetry()
{
    auto* mqttMgr = static_cast<MQTTManager*>(context->getManager("MQTTManager"));
    if (!mqttMgr || !mqttMgr->isConnected()) {
        return;  // offline: telemetry for this tick is simply skipped
    }
    String json = buildTelemetryJson();
    mqttMgr->publish(getHostname() + "/telemetry", json, true /*retain*/, false, false);
}

String DiagnosticsManager::buildTelemetryJson()
{
    uint32_t freeHeap = ESP.getFreeHeap();
#ifdef ESP32
    uint32_t minFree = ESP.getMinFreeHeap();
    uint32_t maxAlloc = ESP.getMaxAllocHeap();
#else
    uint32_t minFree = freeHeap;   // ESP8266: pas de min-free natif
    uint32_t maxAlloc = freeHeap;
#endif
    // Fragmentation: 0% = one big free block, ->100% = heavily fragmented.
    int fragPct = (freeHeap > 0) ? (int)(100 - ((uint64_t)maxAlloc * 100 / freeHeap)) : 0;
    if (fragPct < 0) fragPct = 0;

    auto* wifiMgr = static_cast<WiFiManager*>(context->getManager("WiFiManager"));
    long rssi = (wifiMgr && wifiMgr->isConnected()) ? WiFi.RSSI() : 0;
    unsigned long wifiReconnects = wifiMgr ? wifiMgr->getReconnectCount() : 0;

    String json = "{";
    json += "\"freeHeap\":" + String(freeHeap);
    json += ",\"minFreeHeap\":" + String(minFree);
    json += ",\"minHeapEver\":" + String(minFreeHeapEver);
    json += ",\"maxAllocHeap\":" + String(maxAlloc);
    json += ",\"fragPct\":" + String(fragPct);
    json += ",\"uptimeSec\":" + String(millis() / 1000);
    json += ",\"rssi\":" + String(rssi);
    json += ",\"wifiReconnects\":" + String(wifiReconnects);
    json += ",\"mqttDisconnects\":" + String(mqttDisconnects);
    json += ",\"bootCount\":" + String(bootCount);
    json += ",\"resetReason\":\"" + lastResetReason + "\"";
    json += ",\"coreDump\":" + String(coreDumpPresent ? "true" : "false");
    json += "}";
    return json;
}

// ---------------------------------------------------------------------------
// Persistent black box (NVS + LittleFS)
// ---------------------------------------------------------------------------

void DiagnosticsManager::persistHeartbeat()
{
#ifdef ESP32
    Preferences prefs;
    if (prefs.begin(NVS_NS, false)) {
        prefs.putULong("lastUptime", millis() / 1000);
        prefs.putULong("minHeap", minFreeHeapEver);
        prefs.end();
    }
#endif
}

void DiagnosticsManager::appendEvent(const String& event)
{
    String line = String(millis() / 1000) + "s " + event + "\n";

    File f = LittleFS.open(EVENTS_PATH, "a");
    if (!f) {
        return;  // FS unavailable: skip silently (still have Serial/telemetry)
    }
    f.print(line);
    size_t size = f.size();
    f.close();

    // Rotate: when the log grows past the cap, keep the most recent half (FIFO).
    if (size > EVENTS_LOG_MAX) {
        File r = LittleFS.open(EVENTS_PATH, "r");
        if (r) {
            r.seek(size / 2, SeekSet);
            String tail = r.readString();
            r.close();
            int nl = tail.indexOf('\n');  // align to a line boundary
            if (nl >= 0) tail = tail.substring(nl + 1);
            File w = LittleFS.open(EVENTS_PATH, "w");
            if (w) {
                w.print(tail);
                w.close();
            }
        }
    }
}

String DiagnosticsManager::readEventsLog()
{
    File f = LittleFS.open(EVENTS_PATH, "r");
    if (!f) {
        return "(no events log)";
    }
    String content = f.readString();
    f.close();
    return content.length() ? content : "(empty)";
}

// ---------------------------------------------------------------------------
// Core dump
// ---------------------------------------------------------------------------

String DiagnosticsManager::buildCoreDumpString()
{
#if defined(ESP32) && defined(DIAG_HAS_COREDUMP)
    if (esp_core_dump_image_check() != ESP_OK) {
        return "(no core dump)";
    }
    String out = "core dump present";
    esp_core_dump_summary_t summary;
    if (esp_core_dump_get_summary(&summary) == ESP_OK) {
        out += "; task=" + String(summary.exc_task);
        char pc[16];
        snprintf(pc, sizeof(pc), "0x%08x", (unsigned)summary.exc_pc);
        out += " PC=" + String(pc);
    }
    out += " (decode: esp-coredump info_corefile -c firmware.elf)";
    return out;
#else
    return "(core dump API unavailable)";
#endif
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

String DiagnosticsManager::buildDumpString()
{
    String s = "=== Diagnostics ===\n";
    s += "Boot count       : " + String(bootCount) + "\n";
    s += "This reset reason: " + lastResetReason + " (" + String(resetReasonCode) + ")\n";
    s += "Prev run uptime  : " + String(lastUptimeSec) + " s\n";
    s += "Current uptime   : " + String(millis() / 1000) + " s\n";
    s += "Free heap        : " + String(ESP.getFreeHeap()) + " bytes\n";
#ifdef ESP32
    s += "Min free (boot)  : " + String(ESP.getMinFreeHeap()) + " bytes\n";
#else
    s += "Min free (boot)  : " + String(ESP.getFreeHeap()) + " bytes\n";
#endif
    s += "Min heap ever    : " + String(minFreeHeapEver) + " bytes\n";
    s += "WiFi reconnects  : ";
    auto* wifiMgr = static_cast<WiFiManager*>(context->getManager("WiFiManager"));
    s += String(wifiMgr ? wifiMgr->getReconnectCount() : 0) + "\n";
    s += "MQTT disconnects : " + String(mqttDisconnects) + "\n";
    s += "Core dump        : " + (coreDumpPresent ? coreDumpSummary : String("none")) + "\n";
    s += "--- recent events ---\n";
    s += readEventsLog();
    return s;
}

void DiagnosticsManager::registerCommands()
{
    auto* cmdMgr = static_cast<CommandManager*>(context ? context->getManager("CommandManager") : nullptr);
    if (!cmdMgr) {
        debug("CommandManager not available for diag commands", 0);
        return;
    }

    cmdMgr->registerCommand(Command(
        "diag", "dump", "Show diagnostics black box (reset reason, heap, events)",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String { return buildDumpString(); }));

    cmdMgr->registerCommand(Command(
        "diag", "coredump", "Show summary of the last crash core dump",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String { return buildCoreDumpString(); }));

    cmdMgr->registerCommand(Command(
        "diag", "clear", "Erase diagnostics black box (counters, events, core dump)",
        CommandSource::Any, true,
        [this](const std::vector<String>& args) -> String {
            LittleFS.remove(EVENTS_PATH);
#ifdef ESP32
            Preferences prefs;
            if (prefs.begin(NVS_NS, false)) { prefs.clear(); prefs.end(); }
#if defined(DIAG_HAS_COREDUMP)
            esp_core_dump_image_erase();
#endif
#endif
            coreDumpPresent = false;
            bootCount = 0;
            mqttDisconnects = 0;
            minFreeHeapEver = ESP.getFreeHeap();
            return "Diagnostics black box cleared";
        }));

    debug("Diagnostics commands registered (diag:dump, diag:coredump, diag:clear)", 1);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

String DiagnosticsManager::getHostname()
{
    auto* configMgr = static_cast<ConfigurationManager*>(context ? context->getManager("ConfigurationManager") : nullptr);
    return configMgr ? configMgr->getHostname() : String("ESP32");
}

String DiagnosticsManager::resetReasonString(int reason)
{
#ifdef ESP32
    switch ((esp_reset_reason_t)reason) {
        case ESP_RST_POWERON:   return "POWERON";
        case ESP_RST_EXT:       return "EXT_PIN";
        case ESP_RST_SW:        return "SW_RESET";
        case ESP_RST_PANIC:     return "PANIC";       // exception/abort -> core dump
        case ESP_RST_INT_WDT:   return "INT_WDT";     // interrupt watchdog
        case ESP_RST_TASK_WDT:  return "TASK_WDT";    // our task watchdog
        case ESP_RST_WDT:       return "OTHER_WDT";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
        case ESP_RST_BROWNOUT:  return "BROWNOUT";    // power supply issue
        case ESP_RST_SDIO:      return "SDIO";
        default:                return "UNKNOWN";
    }
#else
    return "UNKNOWN";
#endif
}
