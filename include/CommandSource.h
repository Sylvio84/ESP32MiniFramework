#ifndef COMMAND_SOURCE_H
#define COMMAND_SOURCE_H

/**
 * @brief Enumeration of command sources
 * Used to control which channels can execute specific commands
 */
enum class CommandSource {
    Any     = 0xFF,  // Command can be executed from any source
    Serial  = 0x01,  // Command from Serial/USB console
    MQTT    = 0x02,  // Command from MQTT topic
    Telnet  = 0x04,  // Command from Telnet connection
    Web     = 0x08,  // Command from Web interface
    Internal = 0x10  // Command from internal system
};

/**
 * @brief Check if a command source matches allowed sources
 * @param allowed Allowed sources (can be OR'ed together)
 * @param actual Actual source of the command
 * @return true if the actual source is allowed
 */
inline bool isSourceAllowed(CommandSource allowed, CommandSource actual) {
    if (allowed == CommandSource::Any) return true;
    return (static_cast<int>(allowed) & static_cast<int>(actual)) != 0;
}

/**
 * @brief Convert command source to string for display
 * @param source Command source
 * @return String representation
 */
inline String commandSourceToString(CommandSource source) {
    switch(source) {
        case CommandSource::Serial: return "Serial";
        case CommandSource::MQTT: return "MQTT";
        case CommandSource::Telnet: return "Telnet";
        case CommandSource::Web: return "Web";
        case CommandSource::Internal: return "Internal";
        case CommandSource::Any: return "Any";
        default: return "Unknown";
    }
}

#endif // COMMAND_SOURCE_H