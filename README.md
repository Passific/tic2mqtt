# tic2mqtt


A C++ application to read data from a serial port, extract the data, and export it to an MQTT server. Designed for Home Assistant MQTT Discovery integration.


## Features
- Serial port reading (using [serial](https://github.com/wjwwood/serial))
- Data parsing (TIC Standard and Historique modes)
- MQTT publishing (using [Paho MQTT C++](https://github.com/eclipse/paho.mqtt.cpp))
- Home Assistant MQTT Discovery support
- Modular codebase (all TIC logic in `src/tic/`)

## Build Instructions


1. Install dependencies:
   - CMake
   - A C++17 compiler (MSVC, GCC, or Clang)
   - [Paho MQTT C++ library](https://github.com/eclipse/paho.mqtt.cpp) (and its C dependency)
   - [serial library](https://github.com/wjwwood/serial)
   - On Windows, you can use [vcpkg](https://github.com/microsoft/vcpkg):
     ```sh
     vcpkg install paho-mqttpp3 paho-mqtt serial
     ```

2. Configure and build:
   ```sh
   mkdir build
   cd build
   cmake ..
   cmake --build .
   ```

3. Run the application:
   ```sh
   ./tic2mqtt
   ```


## Configuration
- Serial port and MQTT settings can be set in the source code or via environment variables (see `src/tic/tic_utils.cpp`).


## Project Structure

- `src/main.cpp` - Main application logic
- `src/serial_reader.cpp`/`.h` - Serial port reading logic
- `src/mqtt_publisher.cpp`/`.h` - MQTT publishing logic
- `src/tic/` - All TIC protocol logic (Standard/History modes, utils, discovery)


## Docker

You can build and run the application in a container using Docker:

### Build the Docker image
```sh
docker build -t tic2mqtt .
```

### Run the container
```sh
docker run --rm \
   --device=/dev/ttyUSB0 \
   -e MQTT_SERVER=your-mqtt-server \
   -e MQTT_USER=your-mqtt-user \
   -e MQTT_PASS=your-mqtt-password \
   -e SERIAL_PORT=/dev/ttyUSB0 \
   tic2mqtt
```

Adjust device and environment variables as needed for your setup.

## Docker Compose

You can also use Docker Compose to manage the application and its dependencies:

```yaml
services:
   tic2mqtt:
      build: .
      image: tic2mqtt:latest
      restart: unless-stopped
      environment:
         MQTT_SERVER: your-mqtt-server
         MQTT_USER: your-mqtt-user
         MQTT_PASS: your-mqtt-password
         SERIAL_PORT: /dev/ttyUSB0
      devices:
         - /dev/ttyUSB0:/dev/ttyUSB0
```

Adjust device and environment variables as needed for your setup.

## License
MIT
