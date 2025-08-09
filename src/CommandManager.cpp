#include "CommandManager.h"
#include "FrameworkContext.h"
#include <sstream>

CommandManager::CommandManager(FrameworkContext& context) : Manager(context) {
}

void CommandManager::init() {
    debug("Initializing CommandManager");
    
    // Register built-in commands
    registerBuiltInCommands();
    
    setInitialized(true);
    debug("CommandManager initialized");
}

void CommandManager::registerBuiltInCommands() {
    // General help command - shows all namespaces
    Command generalHelpCmd;
    generalHelpCmd.namespaceName = "";  // No namespace for general help
    generalHelpCmd.name = "help";
    generalHelpCmd.description = "Show available command namespaces and help";
    generalHelpCmd.source = CommandSource::Any;
    generalHelpCmd.historyEnabled = false;
    generalHelpCmd.execute = [this](const std::vector<String>& args) {
        String result = "Available Command Namespaces:\n\n";
        result += "  sys      - System commands (version, uptime, restart, etc.)\n";
        result += "  wifi     - WiFi connection management\n";  
        result += "  mqtt     - MQTT broker communication\n";
        result += "  time     - Date/time and scheduling\n";
        result += "  config   - Configuration management\n";
        result += "  device   - Device management\n";
        result += "  program  - Device program management\n";
        result += "  espui    - Web interface commands\n";
        result += "  command  - Command system management\n\n";
        result += "Usage:\n";
        result += "  <namespace>:help  - Show commands for specific namespace\n";
        result += "  <namespace>:<cmd> - Execute command\n\n";
        result += "Examples:\n";
        result += "  help         - Show this help\n";
        result += "  sys:help     - Show system commands\n";
        result += "  wifi:status  - Show WiFi status\n";
        result += "  command:list - List all available commands";
        return result;
    };
    registerCommand(generalHelpCmd);
    
    // Aliases for general help
    registerAlias("?", "help");
    
    // List command
    Command listCmd;
    listCmd.namespaceName = "command";
    listCmd.name = "list";
    listCmd.description = "List all commands";
    listCmd.source = CommandSource::Any;
    listCmd.historyEnabled = false;
    listCmd.execute = [this](const std::vector<String>& args) {
        String filter = "";
        if (args.size() > 0) {
            filter = args[0];
        }
        
        String result = "Available commands";
        if (!filter.isEmpty()) {
            result += " (filter: " + filter + ")";
        }
        result += ":\n";
        
        auto cmds = listAvailable(CommandSource::Any, filter);
        for (const auto& cmd : cmds) {
            result += "  " + cmd.getFullName();
            if (!cmd.description.isEmpty()) {
                result += " - " + cmd.description;
            }
            result += "\n";
        }
        
        return result;
    };
    registerCommand(listCmd);
    registerAlias("list", "command:list");
    
    // Alias command
    Command aliasCmd;
    aliasCmd.namespaceName = "command";
    aliasCmd.name = "alias";
    aliasCmd.description = "Show all aliases";
    aliasCmd.source = CommandSource::Any;
    aliasCmd.historyEnabled = false;
    aliasCmd.execute = [this](const std::vector<String>& args) {
        String result = "Registered aliases:\n";
        for (const auto& pair : aliases) {
            result += "  " + pair.first + " -> " + pair.second + "\n";
        }
        if (aliases.empty()) {
            result = "No aliases registered";
        }
        return result;
    };
    registerCommand(aliasCmd);
    
    
    // History command
    Command historyCmd;
    historyCmd.namespaceName = "command";
    historyCmd.name = "history";
    historyCmd.description = "Show command history";
    historyCmd.source = CommandSource::Any;
    historyCmd.historyEnabled = false;
    historyCmd.execute = [this](const std::vector<String>& args) {
        String cmdName = "";
        if (args.size() > 0) {
            cmdName = resolveCommandName(args[0]);
        }
        
        if (cmdName.isEmpty()) {
            // Show all commands with history
            String result = "Command history:\n";
            for (const auto& pair : history) {
                if (!pair.second.empty()) {
                    result += "\n" + pair.first + ":\n";
                    for (const auto& entry : pair.second) {
                        result += "  [" + String(entry.timestamp) + "] ";
                        result += "from " + commandSourceToString(entry.source) + ": ";
                        for (const auto& arg : entry.arguments) {
                            result += arg + " ";
                        }
                        result += "-> " + entry.result + "\n";
                    }
                }
            }
            return result.isEmpty() ? "No command history" : result;
        } else {
            // Show history for specific command
            auto entries = getHistory(cmdName);
            if (entries.empty()) {
                return String("No history for command: " + cmdName);
            }
            
            String result = "History for " + cmdName + ":\n";
            for (const auto& entry : entries) {
                result += "  [" + String(entry.timestamp) + "] ";
                result += "from " + commandSourceToString(entry.source) + ": ";
                for (const auto& arg : entry.arguments) {
                    result += arg + " ";
                }
                result += "-> " + entry.result + "\n";
            }
            return result;
        }
    };
    registerCommand(historyCmd);
}

bool CommandManager::registerCommand(const Command& command) {
    String fullName = command.getFullName();
    
    if (commands.find(fullName) != commands.end()) {
        debug("Command already exists: " + fullName, 1);
        return false;
    }
    
    commands[fullName] = command;
    debug("Registered command: " + fullName, 2);
    return true;
}

bool CommandManager::registerAlias(const String& alias, const String& fullCommandName) {
    // Check if alias already exists
    if (aliases.find(alias) != aliases.end()) {
        debug("Alias already exists: " + alias, 1);
        return false;
    }
    
    // Check if command exists
    if (commands.find(fullCommandName) == commands.end()) {
        debug("Command not found for alias: " + fullCommandName, 1);
        return false;
    }
    
    aliases[alias] = fullCommandName;
    debug("Registered alias: " + alias + " -> " + fullCommandName, 2);
    return true;
}

String CommandManager::resolveCommandName(const String& nameOrAlias) {
    // Check if it's an alias
    auto aliasIt = aliases.find(nameOrAlias);
    if (aliasIt != aliases.end()) {
        return aliasIt->second;
    }
    
    // Check if it's a full command name
    if (commands.find(nameOrAlias) != commands.end()) {
        return nameOrAlias;
    }
    
    // Try to find command without namespace
    for (const auto& pair : commands) {
        if (pair.second.name == nameOrAlias) {
            return pair.first;
        }
    }
    
    return ""; // Not found
}

void CommandManager::parseCommandInput(const String& input, String& commandName, 
                                      std::vector<String>& args) {
    args.clear();
    
    // Trim input
    String trimmed = input;
    trimmed.trim();
    
    if (trimmed.isEmpty()) {
        commandName = "";
        return;
    }
    
    // Find first space
    int spaceIndex = trimmed.indexOf(' ');
    
    if (spaceIndex == -1) {
        // No arguments
        commandName = trimmed;
        return;
    }
    
    // Extract command name
    commandName = trimmed.substring(0, spaceIndex);
    
    // Extract arguments
    String argsStr = trimmed.substring(spaceIndex + 1);
    argsStr.trim();
    
    // Simple argument parsing (space-separated)
    // TODO: Add support for quoted arguments
    while (!argsStr.isEmpty()) {
        int nextSpace = argsStr.indexOf(' ');
        if (nextSpace == -1) {
            args.push_back(argsStr);
            break;
        } else {
            String arg = argsStr.substring(0, nextSpace);
            if (!arg.isEmpty()) {
                args.push_back(arg);
            }
            argsStr = argsStr.substring(nextSpace + 1);
            argsStr.trim();
        }
    }
}

String CommandManager::executeCommand(const String& nameOrAlias,
                                     const std::vector<String>& args,
                                     CommandSource source) {
    // Resolve command name
    String fullName = resolveCommandName(nameOrAlias);
    
    if (fullName.isEmpty()) {
        return "Command not found: " + nameOrAlias;
    }
    
    // Get command
    auto cmdIt = commands.find(fullName);
    if (cmdIt == commands.end()) {
        return "Command not found: " + fullName;
    }
    
    const Command& cmd = cmdIt->second;
    
    // Check source permission
    if (!cmd.isAllowedFrom(source)) {
        return "Command not allowed from " + commandSourceToString(source) + ": " + fullName;
    }
    
    // Execute command
    String result;
    if (cmd.execute) {
        try {
            result = cmd.execute(args);
        } catch (...) {
            result = "Error executing command: " + fullName;
        }
    } else {
        result = "Command has no execution handler: " + fullName;
    }
    
    // Add to history if enabled
    if (cmd.historyEnabled) {
        addToHistory(fullName, args, result, source);
    }
    
    return result;
}

String CommandManager::executeCommandString(const String& input, CommandSource source) {
    String commandName;
    std::vector<String> args;
    
    parseCommandInput(input, commandName, args);
    
    if (commandName.isEmpty()) {
        return "Empty command";
    }
    
    return executeCommand(commandName, args, source);
}

void CommandManager::addToHistory(const String& fullName, const std::vector<String>& args,
                                 const String& result, CommandSource source) {
    CommandHistoryEntry entry;
    entry.timestamp = millis();
    entry.arguments = args;
    entry.result = result;
    entry.source = source;
    
    auto& entries = history[fullName];
    entries.push_back(entry);
    
    // Limit history size
    while (entries.size() > MAX_HISTORY_ENTRIES) {
        entries.erase(entries.begin());
    }
}

std::vector<Command> CommandManager::listAvailable(CommandSource source,
                                                  const String& namespaceFilter) {
    std::vector<Command> result;
    
    for (const auto& pair : commands) {
        const Command& cmd = pair.second;
        
        // Check source filter
        if (source != CommandSource::Any && !cmd.isAllowedFrom(source)) {
            continue;
        }
        
        // Check namespace filter
        if (!namespaceFilter.isEmpty() && cmd.namespaceName != namespaceFilter) {
            continue;
        }
        
        result.push_back(cmd);
    }
    
    return result;
}

std::vector<CommandHistoryEntry> CommandManager::getHistory(const String& commandName) {
    auto it = history.find(commandName);
    if (it != history.end()) {
        return it->second;
    }
    return std::vector<CommandHistoryEntry>();
}

void CommandManager::clearHistory(const String& commandName) {
    if (commandName == "*") {
        history.clear();
    } else {
        history.erase(commandName);
    }
}

String CommandManager::getHelp(CommandSource source, const String& filter) {
    String result = "\n=== Command Help ===\n";
    
    // Group commands by namespace
    std::map<String, std::vector<Command>> byNamespace;
    
    for (const auto& pair : commands) {
        const Command& cmd = pair.second;
        
        // Apply source filter
        if (source != CommandSource::Any && !cmd.isAllowedFrom(source)) {
            continue;
        }
        
        // Apply text filter
        if (!filter.isEmpty()) {
            if (cmd.getFullName().indexOf(filter) == -1 &&
                cmd.description.indexOf(filter) == -1) {
                continue;
            }
        }
        
        byNamespace[cmd.namespaceName].push_back(cmd);
    }
    
    // Display commands by namespace
    for (const auto& nsPair : byNamespace) {
        String ns = nsPair.first.isEmpty() ? "global" : nsPair.first;
        result += "\n[" + ns + "]\n";
        
        for (const auto& cmd : nsPair.second) {
            result += "  " + cmd.name;
            
            // Show sources if restricted
            if (cmd.source != CommandSource::Any) {
                result += " (" + commandSourceToString(cmd.source) + ")";
            }
            
            if (!cmd.description.isEmpty()) {
                result += " - " + cmd.description;
            }
            
            result += "\n";
        }
    }
    
    // Show aliases
    if (!aliases.empty()) {
        result += "\n[Aliases]\n";
        for (const auto& pair : aliases) {
            result += "  " + pair.first + " -> " + pair.second + "\n";
        }
    }
    
    result += "\nUse 'command::list [namespace]' to list commands\n";
    result += "Use 'command::history [command]' to view history\n";
    
    return result;
}

bool CommandManager::commandExists(const String& nameOrAlias) {
    return !resolveCommandName(nameOrAlias).isEmpty();
}

bool CommandManager::removeCommand(const String& fullName) {
    auto it = commands.find(fullName);
    if (it != commands.end()) {
        commands.erase(it);
        
        // Remove associated aliases
        std::vector<String> aliasesToRemove;
        for (const auto& pair : aliases) {
            if (pair.second == fullName) {
                aliasesToRemove.push_back(pair.first);
            }
        }
        for (const auto& alias : aliasesToRemove) {
            aliases.erase(alias);
        }
        
        // Clear history
        history.erase(fullName);
        
        return true;
    }
    return false;
}

bool CommandManager::removeAlias(const String& alias) {
    return aliases.erase(alias) > 0;
}

bool CommandManager::onCommand(const String& command, 
                              const std::vector<String>& params) {
    // Handle direct command execution
    // This allows CommandManager to process commands sent to it via the framework
    String result = executeCommand(command, params, CommandSource::Internal);
    
    // Log result if it's an error
    if (result.startsWith("Command not found") || result.startsWith("Error")) {
        debug(result, 1);
        return false;
    }
    
    debug(result, 2);
    return true;
}

String CommandManager::extractNamespace(const String& fullName) {
    int colonIndex = fullName.indexOf(':');
    if (colonIndex == -1) {
        return "";  // No namespace (global command)
    }
    return fullName.substring(0, colonIndex);
}

std::vector<String> CommandManager::getAllNamespaces() {
    std::vector<String> namespaces;
    
    for (const auto& pair : commands) {
        String ns = extractNamespace(pair.first);
        if (ns.isEmpty()) continue;  // Skip global commands
        
        // Check if namespace already exists
        bool found = false;
        for (const String& existing : namespaces) {
            if (existing == ns) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            namespaces.push_back(ns);
        }
    }
    
    return namespaces;
}

String CommandManager::generateNamespaceHelp(const String& namespaceName) {
    String result = namespaceName.c_str();
    result[0] = toupper(result[0]); // Capitalize first letter
    result += " Commands:\n";
    
    // Collect commands for this namespace
    std::vector<Command> namespaceCommands;
    for (const auto& pair : commands) {
        const Command& cmd = pair.second;
        if (cmd.namespaceName == namespaceName && cmd.name != "help") {
            namespaceCommands.push_back(cmd);
        }
    }
    
    // Sort commands by name for consistent output
    std::sort(namespaceCommands.begin(), namespaceCommands.end(), 
              [](const Command& a, const Command& b) {
                  return a.name < b.name;
              });
    
    // Format commands
    for (const auto& cmd : namespaceCommands) {
        result += "  " + namespaceName + ":" + cmd.name;
        
        // Add padding for alignment (simple approach)
        int padding = 20 - (namespaceName.length() + cmd.name.length() + 1);
        for (int i = 0; i < padding && i < 15; i++) {
            result += " ";
        }
        
        result += "- " + cmd.description + "\n";
    }
    
    // Add help command reference
    result += "  " + namespaceName + ":help";
    int helpPadding = 20 - (namespaceName.length() + 4 + 1);
    for (int i = 0; i < helpPadding && i < 15; i++) {
        result += " ";
    }
    result += "- Show this help\n";
    
    // Show aliases for this namespace
    std::vector<String> namespaceAliases;
    for (const auto& aliasPair : aliases) {
        String targetNs = extractNamespace(aliasPair.second);
        if (targetNs == namespaceName) {
            namespaceAliases.push_back(aliasPair.first + " -> " + aliasPair.second);
        }
    }
    
    if (!namespaceAliases.empty()) {
        result += "\nAliases: ";
        for (size_t i = 0; i < namespaceAliases.size(); i++) {
            if (i > 0) result += ", ";
            result += namespaceAliases[i];
        }
    }
    
    return result;
}

void CommandManager::generateHelpCommands() {
    debug("Generating automatic help commands for all namespaces", 2);
    
    std::vector<String> namespaces = getAllNamespaces();
    
    for (const String& ns : namespaces) {
        String helpCommandName = ns + ":help";
        
        // Skip if help command already exists for this namespace
        if (commands.find(helpCommandName) != commands.end()) {
            debug("Help command already exists for namespace: " + ns, 3);
            continue;
        }
        
        // Create the automatic help command
        Command helpCmd;
        helpCmd.namespaceName = ns;
        helpCmd.name = "help";
        helpCmd.description = "Show all " + ns + " commands";
        helpCmd.source = CommandSource::Any;
        helpCmd.historyEnabled = false;
        helpCmd.execute = [this, ns](const std::vector<String>& args) -> String {
            return generateNamespaceHelp(ns);
        };
        
        registerCommand(helpCmd);
        debug("Generated automatic help command: " + helpCommandName, 2);
    }
    
    debug("Automatic help generation completed for " + String(namespaces.size()) + " namespaces", 1);
}