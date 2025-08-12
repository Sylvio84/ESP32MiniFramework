#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include <Arduino.h>
#include <map>
#include <vector>
#include <memory>
#include "Manager.h"
#include <Managers/SerialManager.h>
#include "Command.h"

/**
 * @brief CommandManager - Advanced command registration, execution, and management
 * 
 * Provides a robust, extensible command system that forms the core of user
 * interaction with the ESP32MiniFramework.
 * 
 * ## Key Features:
 * 
 * ### Command Organization
 * - **Namespaces**: Commands organized by namespace (system, wifi, mqtt, config, etc.)
 * - **Full Names**: Commands use format `namespace:command` for clarity
 * - **Aliases**: Short aliases for frequently used commands (e.g., 'v' for 'system:version')
 * - **Auto-discovery**: Commands can be discovered via help system
 * 
 * ### Command Execution
 * - **Source Control**: Commands can be restricted by source (Serial, MQTT, Telnet)
 * - **Parameter Parsing**: Automatic parsing of command arguments
 * - **Callback System**: Commands execute via registered callbacks
 * - **Result Handling**: Commands return string results for display
 * 
 * ### Advanced Features
 * - **Command History**: Optional tracking of command execution history
 * - **Tab Completion**: Support for command auto-completion (future)
 * - **Help System**: Built-in help with categorized command listing
 * - **Permission Levels**: Commands can require authentication (future)
 * 
 * ## Architecture:
 * - **Decoupled Design**: Completely independent from other managers
 * - **Callback Pattern**: Commands encapsulate business logic in callbacks
 * - **Registry Pattern**: Central command and alias registries
 * - **SOLID Compliance**: Single responsibility, open for extension
 * 
 * ## Usage:
 * ```cpp
 * // Register a command
 * cmdMgr->registerCommand(Command(
 *     "system", "version", "Show version",
 *     CommandSource::Any, false,
 *     [](const std::vector<String>& args) -> String {
 *         return "Version 1.0.0";
 *     }
 * ));
 * 
 * // Register an alias
 * cmdMgr->registerAlias("v", "system:version");
 * 
 * // Execute a command
 * String result = cmdMgr->executeCommand("version", {}, CommandSource::Serial);
 * ```
 * 
 * ## Commands Handled:
 * - `help [namespace]` - Display available commands, optionally filtered by namespace
 * - `history [command]` - Show command execution history
 * - `alias [name] [command]` - Create or list command aliases
 * 
 * ## Integration:
 * - Registered with FrameworkContext as "CommandManager"
 * - Called by MainController::processInput() for command execution
 * - Other managers register their commands during init()
 */
class CommandManager : public Manager {
private:
    // Simple history entry with ID and command
    struct HistoryEntry {
        unsigned int id;
        String command;
    };
    
    // Command registry: full_name -> command
    std::map<String, Command> commands;
    
    // Alias registry: alias -> full_name
    std::map<String, String> aliases;
    
    // Command history with unique IDs
    std::vector<HistoryEntry> history;
    
    // Next history ID (incremental)
    unsigned int nextHistoryId = 1;
    
    // Maximum history entries
    static const size_t MAX_HISTORY_ENTRIES = 100;
    
    // Input request management (simple)
    String activeInputPrompt;
    std::function<void(const String&)> activeInputCallback;
    bool waitingForInput = false;
    
    /**
     * @brief Add command to history
     */
    void addToHistory(const String& commandLine);
    
    /**
     * @brief Parse command input to extract name and arguments
     * @param input Raw command string
     * @param commandName Output: extracted command name
     * @param args Output: extracted arguments
     */
    void parseCommandInput(const String& input, String& commandName, 
                          std::vector<String>& args);
    
    /**
     * @brief Resolve alias to full command name
     * @param nameOrAlias Input name or alias
     * @return Full command name
     */
    String resolveCommandName(const String& nameOrAlias);
    
    /**
     * @brief Process history recall commands (!n, !!)
     * @param input The input string starting with !
     * @return The recalled command or empty string if not found
     */
    String processHistoryRecall(const String& input) const;

public:
    /**
     * @brief Constructor
     * @param context Framework context for dependency injection
     */
    explicit CommandManager(FrameworkContext& context);
    
    /**
     * @brief Initialize the command manager
     */
    void init() override;
    
    /**
     * @brief Get manager name
     * @return "CommandManager"
     */
    String getName() const override { return "CommandManager"; }
    
    /**
     * @brief Register a new command
     * @param command Command to register
     * @return true if successful, false if command already exists
     */
    bool registerCommand(const Command& command);
    
    /**
     * @brief Register an alias for a command
     * @param alias Short name
     * @param fullCommandName Full command name (namespace::command)
     * @return true if successful, false if alias exists or command not found
     */
    bool registerAlias(const String& alias, const String& fullCommandName);
    
    /**
     * @brief Execute a command
     * @param nameOrAlias Command name or alias
     * @param args Command arguments
     * @param source Source of the command
     * @return Execution result or error message
     */
    String executeCommand(const String& nameOrAlias, 
                         const std::vector<String>& args,
                         CommandSource source);
    
    /**
     * @brief Execute a command from raw input string
     * @param input Raw command string (e.g., "system::reboot now")
     * @param source Source of the command
     * @return Execution result or error message
     */
    String executeCommandString(const String& input, CommandSource source);
    
    /**
     * @brief List available commands
     * @param source Filter by source (only show commands available from this source)
     * @param namespaceFilter Optional namespace filter
     * @return List of commands with descriptions
     */
    std::vector<Command> listAvailable(CommandSource source = CommandSource::Any,
                                       const String& namespaceFilter = "");
    
    /**
     * @brief Get command history
     * @return History entries
     */
    std::vector<HistoryEntry> getHistory() const;
    
    /**
     * @brief Get command from history by ID
     * @param id History ID
     * @return Command string or empty if not found
     */
    String getHistoryCommand(unsigned int id) const;
    
    /**
     * @brief Clear command history
     */
    void clearHistory();
    
    /**
     * @brief Get help text for all or specific commands
     * @param source Source requesting help
     * @param filter Optional namespace or command filter
     * @return Formatted help text
     */
    String getHelp(CommandSource source = CommandSource::Any,
                   const String& filter = "");
    
    /**
     * @brief Get list of all registered aliases
     * @return Map of alias -> full command name
     */
    std::map<String, String> getAliases() const { return aliases; }
    
    /**
     * @brief Check if a command exists
     * @param nameOrAlias Command name or alias
     * @return true if command exists
     */
    bool commandExists(const String& nameOrAlias);
    
    /**
     * @brief Remove a command
     * @param fullName Full command name
     * @return true if removed, false if not found
     */
    bool removeCommand(const String& fullName);
    
    /**
     * @brief Remove an alias
     * @param alias Alias to remove
     * @return true if removed, false if not found
     */
    bool removeAlias(const String& alias);
    
    /**
     * @brief Request user input with a callback
     * @param prompt Message to display to the user  
     * @param callback Function to call with the user's input (empty string = cancelled)
     * @return true if request started, false if another request is active
     */
    bool requestInput(const String& prompt, 
                     std::function<void(const String&)> callback);
    
    /**
     * @brief Cancel the active input request
     */
    void cancelInput();
    
    /**
     * @brief Check if waiting for input
     * @return true if an input request is active
     */
    bool isWaitingForInput() const { return waitingForInput; }
    
    /**
     * @brief Handle framework commands (help, list, etc.)
     * @param command Command name
     * @param params Command parameters
     * @return true if handled
     */
    bool onCommand(const String& command, 
                  const std::vector<String>& params) override;
    
    /**
     * @brief Handle events from EventManager
     */
    bool onEvent(const String& type, const String& event, 
                const std::vector<String>& params) override;
    
    /**
     * @brief Generate automatic help commands for all namespaces
     * This creates <namespace>:help commands automatically
     */
    void generateHelpCommands();
    
private:
    /**
     * @brief Register built-in commands (help, list, etc.)
     */
    void registerBuiltInCommands();
    
    /**
     * @brief Extract namespace from full command name
     * @param fullName Command full name (e.g., "wifi:status")
     * @return Namespace (e.g., "wifi")
     */
    String extractNamespace(const String& fullName);
    
    /**
     * @brief Generate help text for a specific namespace
     * @param namespaceName The namespace to generate help for
     * @return Formatted help text for the namespace
     */
    String generateNamespaceHelp(const String& namespaceName);
    
    /**
     * @brief Get all unique namespaces from registered commands
     * @return Set of namespace names
     */
    std::vector<String> getAllNamespaces();
};

#endif // COMMAND_MANAGER_H