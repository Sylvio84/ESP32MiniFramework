## Serial commands for test

Available commands via serial (use with esp32cmd.py):

## Test Commands

```bash
# Run unit tests (if implemented)
pio test

# Run the command "sys:info" on the ESP32
esp32cmd.py sys:echo Hello World !

# Send a message to mqtt
esp32mqtt.py esp32test/cmd/sys/echo Hello World !
```

## More commands

```bash
# System information and control
esp32cmd.py sys:info              # Get complete system information
esp32cmd.py sys:version           # Get framework version
esp32cmd.py sys:uptime            # Get system uptime in seconds
esp32cmd.py sys:temp              # Get CPU temperature
esp32cmd.py sys:echo Hello World  # Echo back the provided text
esp32cmd.py sys:restart           # Restart the ESP32
esp32cmd.py sys:debuglevel 3      # Change debug level to 3 (0 to 3)

# LED control
esp32cmd.py sys:led on            # Turn internal LED on
esp32cmd.py sys:led off           # Turn internal LED off
esp32cmd.py sys:led               # Get current LED state

# WiFi commands
esp32cmd.py wifi:status           # Get WiFi connection status
esp32cmd.py wifi:scan             # Scan for available networks
esp32cmd.py wifi:rssi             # Get signal strength

# Other commands
esp32cmd.py help                  # List all available commands
```

## MQTT tests

read docs/mqtt-tests.md