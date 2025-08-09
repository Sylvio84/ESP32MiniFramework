#ifndef COMMAND_H
#define COMMAND_H

#include <Arduino.h>
#include <functional>
#include <vector>
#include "CommandSource.h"

/**
 * @brief Structure representing a command in the system
 * 
 * Commands are lightweight objects with metadata and execution callback.
 * They follow the principle of simplicity and decoupling.
 */
struct Command {
    /**
     * @brief Namespace for logical grouping (e.g., "system", "wifi", "device")
     */
    String namespaceName;
    
    /**
     * @brief Command name (e.g., "reboot", "connect", "status")
     */
    String name;
    
    /**
     * @brief Human-readable description for help system
     */
    String description;
    
    /**
     * @brief Allowed sources for this command (can be OR'ed for multiple sources)
     */
    CommandSource source = CommandSource::Any;
    
    /**
     * @brief Whether to keep history of executions for this command
     */
    bool historyEnabled = false;
    
    /**
     * @brief Execution callback
     * @param args Command arguments
     * @return Result string to send back to the caller
     */
    std::function<String(const std::vector<String>&)> execute;
    
    /**
     * @brief Get the full command name including namespace
     * @return Full name in format "namespace:command"
     */
    String getFullName() const {
        if (namespaceName.isEmpty()) {
            return name;
        }
        return namespaceName + ":" + name;
    }
    
    /**
     * @brief Check if command can be executed from given source
     * @param actualSource The source requesting execution
     * @return true if allowed, false otherwise
     */
    bool isAllowedFrom(CommandSource actualSource) const {
        return isSourceAllowed(source, actualSource);
    }
    
    /**
     * @brief Default constructor
     */
    Command() = default;
    
    /**
     * @brief Full constructor for convenience
     */
    Command(const String& ns, const String& n, const String& desc,
            CommandSource src, bool history,
            std::function<String(const std::vector<String>&)> exec)
        : namespaceName(ns), name(n), description(desc), 
          source(src), historyEnabled(history), execute(exec) {}
};

/**
 * @brief Command execution history entry
 */
struct CommandHistoryEntry {
    unsigned long timestamp;        // millis() when executed
    std::vector<String> arguments;  // Arguments used
    String result;                   // Execution result
    CommandSource source;            // Source of execution
};

#endif // COMMAND_H