cmake_minimum_required(VERSION 3.10)
project(tic2mqtt)

set(CMAKE_CXX_STANDARD 17)

# Find Paho MQTT C++
find_package(PahoMqttCpp REQUIRED)


# Gather all source files
file(GLOB TIC_SOURCES
	src/tic/*.cpp
)
set(SOURCES
	src/main.cpp
	src/serial_reader.cpp
	src/mqtt_publisher.cpp
	${TIC_SOURCES}
)

add_executable(tic2mqtt ${SOURCES})

# Include directories
target_include_directories(tic2mqtt PRIVATE src src/tic)


# Link libraries
target_link_libraries(tic2mqtt PahoMqttCpp::paho-mqttpp3)
