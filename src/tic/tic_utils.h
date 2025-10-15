#include <string>

constexpr const char* MQTT_TOPIC_BASE = "teleinfo/";
#pragma once
#include <string>

std::string sanitize_ascii_printable(const std::string& val);
std::string get_env(const char* var, const char* def);
unsigned long get_env_ulong(const char* var, unsigned long def);
std::string sanitize_label(const std::string& label);
std::string sanitize_value(const std::string& value);
