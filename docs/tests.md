# ESP32MiniFramework Testing Documentation

## Automated Testing (NEW!)

### Quick Test Script
The framework now includes automated test scripts for comprehensive testing:

```bash
# Run quick automated tests (100% success expected)
./test_simple.py

# Run detailed tests with verbose output
./test_esp32.py --verbose

# Run stress test
./test_esp32.py --stress 100

# Run memory leak test
./test_esp32.py --memory
```

### Test Coverage
The automated tests cover:
- **System Commands**: Echo, Info, Help
- **LED Control**: ON, OFF, Toggle, Status
- **WiFi**: Status, SSID
- **MQTT**: Status, Server
- **Configuration**: Set, Get, Clear, List

## Serial Commands for Testing

### Using Python Scripts
If you have the helper scripts available:
```bash
# Using esp32cmd.py (if available)
esp32cmd.py sys:echo Hello World!

# Using esp32mqtt.py for MQTT testing (if available)
esp32mqtt.py esp32test/cmd/sys/echo Hello World!
```

### Direct Serial Connection
```bash
# Connect via screen
screen /dev/ttyACM1 115200

# Or via PlatformIO monitor
pio device monitor
```

## Available Commands

### System Commands
```bash
sys:info              # Get complete system information
sys:version           # Get framework version  
sys:uptime            # Get system uptime
sys:temp              # Get CPU temperature
sys:echo <text>       # Echo back the provided text
sys:restart           # Restart the ESP32
sys:debuglevel <0-3>  # Set debug verbosity level
help                  # List all available command namespaces
```

### LED Control Commands
```bash
led:on                # Turn internal LED on
led:off               # Turn internal LED off
led:toggle            # Toggle LED state
led:status            # Get current LED status
```

### WiFi Commands
```bash
wifi:status           # Get WiFi connection status
wifi:ssid             # Get connected network SSID
wifi:ip               # Get IP address
wifi:scan             # Scan for available networks
wifi:connect <ssid> <password>  # Connect to a network
wifi:disconnect       # Disconnect from WiFi
```

### MQTT Commands
```bash
mqtt:status           # Get MQTT connection status
mqtt:server           # Get MQTT server address
mqtt:connect          # Connect to MQTT broker
mqtt:disconnect       # Disconnect from MQTT broker
mqtt:publish <topic> <message>  # Publish a message
```

### Configuration Commands (NEW!)
```bash
config:get <key>      # Get a configuration value
config:set <key> <value>  # Set a configuration value
config:clear <key>    # Remove a configuration value
config:list           # List all configuration settings
config:hostname       # Get/set device hostname
config:debuglevel     # Get/set debug level
config:power_saving   # Get/set power saving mode
config:json           # Export/import config as JSON
```

## Test Examples

### Basic Connectivity Test
```bash
# Test echo command
sys:echo TEST
# Expected: TEST

# Get system info
sys:info
# Expected: Version info, uptime, memory stats
```

### LED Test Sequence
```bash
led:on
# Expected: LED turned ON

led:status
# Expected: Pin: X State: ON/OFF Active: Yes/No

led:off
# Expected: LED turned OFF

led:toggle
# Expected: LED toggled
```

### Configuration Test
```bash
# Set a value
config:set my_key my_value
# Expected: OK: my_key = my_value

# Get the value
config:get my_key
# Expected: my_key = my_value

# Clear the value
config:clear my_key
# Expected: Cleared: my_key

# Verify it's gone
config:get my_key
# Expected: Key not found: my_key

# List all settings
config:list
# Expected: List of all configuration key-value pairs
```

### Persistent Storage Test
```bash
# Set a value
config:set test_persist 12345

# Restart the device
sys:restart

# After restart, check if value persists
config:get test_persist
# Expected: test_persist = 12345
```

## Running Tests from PlatformIO

### Compile and Upload
```bash
# Build the project
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

### Complete Test Workflow
```bash
# Build, upload and test in one command
pio run --target upload && sleep 3 && ./test_simple.py
```

## Test Results Interpretation

### Success Indicators
- ✓ **100% tests pass**: System working perfectly
- ✓ **>80% tests pass**: System working well
- ⚠ **60-80% tests pass**: Some issues to investigate
- ✗ **<60% tests pass**: Significant issues

### Common Issues and Solutions

1. **Port not found**
   ```bash
   # List available ports
   pio device list
   # Update port in test script if needed
   ```

2. **Permission denied**
   ```bash
   # Add user to dialout group
   sudo usermod -a -G dialout $USER
   # Logout and login again
   ```

3. **Tests failing after upload**
   - Wait 2-3 seconds after upload for device to boot
   - Some commands may not be implemented yet
   - Check debug output with verbose mode

## Debug Levels

Set debug level for more detailed output:
- **0**: Silent (errors only)
- **1**: Basic information
- **2**: Detailed information
- **3**: Verbose (all debug messages)

```bash
config:set debug_level 3
```

## MQTT Testing

For MQTT testing, see [mqtt-tests.md](mqtt-tests.md) for detailed MQTT-specific test scenarios.

## Continuous Integration

The test scripts return appropriate exit codes for CI/CD integration:
- **0**: All tests passed (or >80% for test_simple.py)
- **1**: Tests failed

Example GitHub Actions workflow:
```yaml
- name: Run tests
  run: |
    pio run --target upload
    sleep 3
    ./test_simple.py
```

## Adding New Tests

To add new tests to the automated test suite, edit `test_simple.py`:

```python
("GROUP_NAME", [
    ("Test name", "command", "expected_response"),
    ("Another test", "command", None),  # None = just check for any response
]),
```

## Troubleshooting

### Tests Hang or Timeout
- Check serial connection: `pio device list`
- Ensure correct baudrate: 115200
- Verify USB cable supports data (not charge-only)

### Inconsistent Results
- Add delays between commands if needed
- Clear serial buffers before tests
- Ensure stable power supply

### Command Not Found
- Check available commands with `help`
- Verify firmware version with `sys:info`
- Some commands may be disabled in build flags

## Performance Benchmarks

Expected performance metrics:
- Command response time: <50ms
- Echo round-trip: <20ms
- Config set/get: <30ms
- WiFi status: <100ms

Run performance tests:
```bash
./test_esp32.py --stress 100
```

## Notes

- All configuration values are persistent and survive restarts
- The LED pin is auto-detected based on the ESP32 board model
- Debug messages can be viewed via Serial, Telnet, or MQTT
- Maximum command length: 256 characters
- Maximum config value length: 512 characters