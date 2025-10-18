# tic2mqtt

A Rust application to read data from a serial port, extract the data, and export it to an MQTT server. Designed for Home Assistant MQTT Discovery integration.

## Features

- Serial port reading (using tokio-serial)
- Data parsing (TIC Standard and Historique modes)
- MQTT publishing (using rumqttc)
- Home Assistant MQTT Discovery support
- Graceful shutdown support
- Environment-based configuration

## Build Instructions

### Local Build

1. Install Rust (1.70.0 or later recommended)
   ```sh
   curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
   ```

2. Clone and build:
   ```sh
   git clone https://github.com/passific/tic2mqtt.git
   cd tic2mqtt/rust
   cargo build --release
   ```

3. Run the application:
   ```sh
   ./target/release/tic2mqtt
   ```

### Docker Build

Build and run using Docker:

```sh
# Build
docker build -t tic2mqtt-rust .

# Run
docker run --rm \
   --device=/dev/ttyUSB0 \
   -e MQTT_SERVER=your-mqtt-server \
   -e MQTT_USER=your-mqtt-user \
   -e MQTT_PASS=your-mqtt-password \
   -e SERIAL_PORT=/dev/ttyUSB0 \
   -e TIC_MODE=historique \
   tic2mqtt-rust
```

## Configuration

The application can be configured via command line arguments or environment variables:

### Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `MQTT_SERVER` | MQTT broker address | tcp://localhost:1883 |
| `MQTT_CLIENT_ID` | MQTT client identifier | tic2mqtt_client |
| `MQTT_USER` | MQTT username for authentication | (empty) |
| `MQTT_PASS` | MQTT password for authentication | (empty) |
| `TIC_MODE` | TIC mode (standard or historique) | standard |
| `SERIAL_PORT` | Serial port device path | /dev/ttyUSB0 |

### Command Line Arguments

```
USAGE:
    tic2mqtt [OPTIONS]

OPTIONS:
    --serial <PATH>            Serial device path
    --mqtt-server <URL>        MQTT broker address
    --mqtt-user <USERNAME>     MQTT username
    --mqtt-pass <PASSWORD>     MQTT password
    --mqtt-client-id <ID>      MQTT client identifier
    --mode <MODE>              TIC mode (standard/historique)
    -h, --help                Display this help message
```

Command line arguments take precedence over environment variables.

## Home Assistant Integration

The application automatically sends MQTT discovery messages to Home Assistant. Each TIC label is exposed as a separate sensor with appropriate device class, state class, and unit configuration.

Discovery messages are sent:
- On initial connection
- When Home Assistant comes online
- When new labels are discovered

## Docker Compose Example

```yaml
version: '3'
services:
  tic2mqtt:
    build: 
      context: .
      dockerfile: Dockerfile
    devices:
      - /dev/ttyUSB0:/dev/ttyUSB0
    environment:
      - MQTT_SERVER=tcp://mqtt:1883
      - MQTT_USER=your_username
      - MQTT_PASS=your_password
      - TIC_MODE=historique
      - SERIAL_PORT=/dev/ttyUSB0
    restart: unless-stopped
```

## License

MIT
