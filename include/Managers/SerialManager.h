#ifndef SERIAL_COMMAND_MANAGER_H
#define SERIAL_COMMAND_MANAGER_H

#include <Arduino.h>
#include <FrameworkContext.h>
#include <functional>
#include <map>
#include <vector>
#include "Manager.h"

/**
 * @brief SerialManager - Serial input handling and command triggering
 * 
 * Manages serial communication and converts user input into command execution
 * through the event system.
 * 
 * ## Key Responsibilities:
 * 
 * ### Serial Input Processing
 * - **Character Buffering**: Accumulates characters until newline
 * - **Backspace Support**: Handles character deletion (ASCII 8 and 127)
 * - **Line Termination**: Processes commands on newline (ASCII 10)
 * - **Carriage Return**: Ignores CR characters (ASCII 13)
 * 
 * ### Event Generation
 * - **Command Events**: Triggers "serial/input" event with complete command
 * - **Power Management**: Triggers power saving suspend/resume events
 * - **Empty Input**: Sends power saving events on empty lines
 * 
 * ### Integration Flow
 * 1. SerialManager reads serial input in `loop()`
 * 2. Complete command triggers "serial/input" event
 * 3. MainController receives event and calls `processInput()`
 * 4. `processInput()` uses CommandManager to execute command
 * 5. Result is printed back to Serial
 * 
 * ## Architecture:
 * - **Event-Driven**: Communicates via EventManager events
 * - **Decoupled**: No direct dependency on CommandManager
 * - **Buffer Management**: Maintains input buffer for line assembly
 * - **Non-Blocking**: Processes available characters without blocking
 * 
 * ## Configuration:
 * - **Baud Rate**: 115200 (configurable)
 * - **Input Buffer**: Dynamic string buffer
 * - **Line Endings**: LF or CRLF supported
 * 
 * ## Events Triggered:
 * - `serial:input [command]` - Complete command ready for processing
 * - `sys:power_saving_suspend` - Suspend power saving on input
 * - `sys:power_saving_resume [timeout]` - Resume power saving after timeout
 * 
 * ## Usage Notes:
 * - Must be registered with FrameworkContext as "SerialManager"
 * - Requires Serial.begin() called during init()
 * - loop() must be called periodically to process input
 * - Works with USB serial, Bluetooth serial, etc.
 */
class SerialManager : public Manager
{
  public:
    // Constructor with dependency injection
    SerialManager(FrameworkContext& ctx);

    // Implement Manager interface
    void init() override;
    void loop() override;
    String getName() const override { return "SerialManager"; }

    void addToInputBuffer(const String input, bool resetBuffer = false);
    void output(const String& output);

  private:
    int baudRate = 115200;
    FrameworkContext* context;

    String inputBuffer;

    void handleInput();
};

#endif  // SERIAL_COMMAND_MANAGER_H
