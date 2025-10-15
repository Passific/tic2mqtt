
#include "tic_mode.h"
#include <iostream>

// Helper: generate discovery message (topic, payload) for a label
std::pair<std::string, std::string> make_discovery_message(const TicMode* mode, const std::string& label) {
	std::string safe_label = sanitize_label(label);
	std::string object_id = "tic2mqtt_" + safe_label;
	std::string config_topic = "homeassistant/sensor/" + object_id + "/config";
	std::string state_topic = mode->get_mqtt_topic(label);
	const char* device_class = mode->get_ha_device_class(label);
	const char* state_class = mode->get_ha_state_class(label);
	const char* unit = mode->get_ha_unit(label);
	std::string payload = "{";
	payload += "\"name\": \"TIC " + safe_label + "\",";
	payload += "\"state_topic\": \"" + state_topic + "\",";
	payload += "\"unique_id\": \"" + object_id + "\",";
	if (device_class) payload += "\"device_class\": \"" + std::string(device_class) + "\",";
	if (state_class) payload += "\"state_class\": \"" + std::string(state_class) + "\",";
	if (unit) payload += "\"unit_of_measurement\": \"" + std::string(unit) + "\",";
	if (payload.back() == ',') payload.pop_back();
	payload += "}";
	return {config_topic, payload};
}

std::vector<std::pair<std::string, std::string>> TicMode::get_all_discovery_messages() const {
	std::vector<std::pair<std::string, std::string>> msgs;
	if (meter_id_.empty()) return msgs;
	for (const auto& label : labels()) {
		msgs.push_back(make_discovery_message(this, label));
	}
	return msgs;
}
