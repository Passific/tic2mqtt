#include "tic_mode.h"
#include <iostream>
#include <mqtt/async_client.h>

void TicMode::publish_ha_discovery(mqtt::async_client& client, const std::string& label) const {
    std::string safe_label = sanitize_label(label);
    std::string object_id = "tic2mqtt_" + safe_label;
    std::string config_topic = "homeassistant/sensor/" + object_id + "/config";
    std::string state_topic = get_mqtt_topic(label);
    // Add device_class, state_class, and unit if available
    const char* device_class = get_ha_device_class(label);
    const char* state_class = get_ha_state_class(label);
    const char* unit = get_ha_unit(label);
    std::string payload = "{";
    payload += "\"name\": \"TIC " + safe_label + "\",";
    payload += "\"state_topic\": \"" + state_topic + "\",";
    payload += "\"unique_id\": \"" + object_id + "\",";
    if (device_class) payload += "\"device_class\": \"" + std::string(device_class) + "\",";
    if (state_class) payload += "\"state_class\": \"" + std::string(state_class) + "\",";
    if (unit) payload += "\"unit_of_measurement\": \"" + std::string(unit) + "\",";
    if (payload.back() == ',') payload.pop_back();
    payload += "}";
    mqtt::message_ptr msg = mqtt::make_message(config_topic, payload);
    msg->set_qos(1);
    try {
        client.publish(msg)->wait_for(std::chrono::seconds(2));
    } catch (const mqtt::exception& exc) {
        std::cerr << "MQTT discovery publish failed: " << exc.what() << std::endl;
    }
}
