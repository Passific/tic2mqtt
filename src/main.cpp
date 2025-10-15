#include <mqtt/async_client.h>
#include <iostream>

#include "tic/tic_utils.h"
#include "tic/historique_tic.h"
#include "tic/standard_tic.h"
#include "serial_reader.h"
#include "mqtt_publisher.h"

constexpr auto MQTT_SERVER = "tcp://localhost:1883";
constexpr auto MQTT_CLIENT_ID = "tic2mqtt_client";

// Helper: parse label and value from a TIC line (returns true if both found)
bool parse_label_value(const std::string& line, std::string& label, std::string& value) {
    size_t pos1 = line.find_first_not_of(" \t\r\n");
    if (pos1 == std::string::npos) return false;
    size_t pos2 = line.find_first_of(" \t\r\n", pos1);
    if (pos2 == std::string::npos) return false;
    label = line.substr(pos1, pos2 - pos1);
    size_t pos3 = line.find_first_not_of(" \t\r\n", pos2);
    if (pos3 == std::string::npos) return false;
    size_t pos4 = line.find_first_of(" \t\r\n", pos3);
    value = (pos4 == std::string::npos) ? line.substr(pos3) : line.substr(pos3, pos4 - pos3);
    return !(label.empty() || value.empty());
}

int main() {
    // Read config from environment variables
    std::string serial_port = get_env("SERIAL_PORT", "/dev/ttyUSB0");
    std::string tic_mode = get_env("TIC_MODE", "historique");
    unsigned long baudrate;
    std::unique_ptr<TicMode> mode;
    if (tic_mode == "standard") {
        mode = std::make_unique<StandardTIC>();
        baudrate = StandardTIC::BAUDRATE;
    } else {
        mode = std::make_unique<HistoriqueTIC>();
        baudrate = HistoriqueTIC::BAUDRATE;
    }
    std::string mqtt_server = get_env("MQTT_SERVER", MQTT_SERVER);
    std::string mqtt_client_id = get_env("MQTT_CLIENT_ID", MQTT_CLIENT_ID);
    std::string mqtt_user = get_env("MQTT_USER", "");
    std::string mqtt_pass = get_env("MQTT_PASS", "");

    SerialReader serial(serial_port, baudrate);
    MqttPublisher mqtt(mqtt_server, mqtt_client_id, mqtt_user, mqtt_pass, *mode);
    serial.start();
    mqtt.start();

    // Main loop: pop lines from serial and publish to MQTT

    while (true) {
        std::string line;
        if (serial.pop_line(line)) {
            std::string label, value;
            if (parse_label_value(line, label, value)) {
                // Inform TIC mode of label/value for meter_id tracking
                mode->handle_label_value(label, value);
                mqtt.publish_label_value(label, value);
            } else {
                std::cout << "[Main] Invalid line: " << line;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    serial.stop();
    mqtt.stop();
    return 0;
}
