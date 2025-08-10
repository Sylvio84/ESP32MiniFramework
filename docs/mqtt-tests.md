##Available commands via MQTT (use with esp32mqtt.py):

```bash
# System commands via MQTT
esp32mqtt.py esp32test/cmd/sys/info              # Get system information
esp32mqtt.py esp32test/cmd/sys/version           # Get framework version
esp32mqtt.py esp32test/cmd/sys/uptime            # Get uptime
esp32mqtt.py esp32test/cmd/sys/temp              # Get temperature
esp32mqtt.py esp32test/cmd/sys/echo "Hello"      # Echo text
esp32mqtt.py esp32test/cmd/sys/led on            # LED control
esp32mqtt.py esp32test/cmd/sys/debuglevel 3      # Change debug level to 3 (0 to 3)

# Verbose mode for debugging
esp32mqtt.py esp32test/cmd/sys/info -v           # Show MQTT communication details

# Direct mosquitto commands for testing
mosquitto_pub -h v.zore.org -u sylvio -P [password] -t "esp32test/cmd/sys/echo" -m "Test"
mosquitto_sub -h v.zore.org -u sylvio -P [password] -t "esp32test/log" -v
```

**Important notes:**
- All commands respond on the topic: `esp32test/log`
- Except for device commands and the mqtt status ping response "online" (on `esp32test/status`)
- The ESP32 hostname is `esp32test` by default
- MQTT broker is configured as `v.zore.org` on port 1883 

