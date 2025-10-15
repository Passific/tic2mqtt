#pragma once
#include <string>

/**
 * @file tic_utils.h
 * @brief Utility functions for TIC to MQTT bridge (sanitization, env, etc).
 */

constexpr const char* MQTT_TOPIC_BASE = "homeassistant/sensor/tic2mqtt/";

/**
 * @brief Remove non-ASCII-printable characters from a string.
 * @param val Input string.
 * @return Sanitized string with only ASCII printable characters.
 */
std::string sanitize_ascii_printable(const std::string& val);

/**
 * @brief Get an environment variable as a sanitized string.
 * @param var Environment variable name.
 * @param def Default value if not set.
 * @return Sanitized value of the environment variable or default.
 */
std::string get_env(const char* var, const char* def);

/**
 * @brief Get an environment variable as an unsigned long.
 * @param var Environment variable name.
 * @param def Default value if not set or conversion fails.
 * @return Value as unsigned long or default.
 */
unsigned long get_env_ulong(const char* var, unsigned long def);

/**
 * @brief Sanitize a label for use in MQTT topics (alphanumeric, _, -).
 * @param label Input label string.
 * @return Sanitized label string.
 */
std::string sanitize_label(const std::string& label);

/**
 * @brief Sanitize a value for use in MQTT payloads (ASCII printable only).
 * @param value Input value string.
 * @return Sanitized value string.
 */
std::string sanitize_value(const std::string& value);
