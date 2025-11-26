
#pragma once
#include "tic_utils.h"

#include <string>
#include <set>
#include <vector>
#include <utility>
#include <map>

/**
 * @brief Abstract base class for TIC mode implementations (historique/standard).
 *        Provides label set, Home Assistant helpers, and meter ID tracking.
 */
class TicMode {
protected:
	// Map of all current label/value pairs for the current frame
	std::map<std::string, std::string> label_values_;
public:
		/**
		* @brief Get all current label/value pairs as a map.
		* @return const reference to the label/value map.
		*/
		const std::map<std::string, std::string>& get_label_values() const { return label_values_; }
	/**
	* @brief Virtual destructor.
	*/
	virtual ~TicMode() = default;

	/**
	* @brief Get the set of all supported labels for this TIC mode.
	* @return Reference to the set of label strings.
	*/
	virtual const std::set<std::string>& labels() const = 0;

	/**
	* @brief Set the meter ID (ADCO/ADSC) for topic construction.
	* @param id The meter ID string.
	*/
	void set_meter_id(const std::string& id) { meter_id_ = id; }

	/**
	* @brief Get the current meter ID.
	* @return Reference to the meter ID string.
	*/
	const std::string& get_meter_id() const { return meter_id_; }

	/**
	* @brief Handle a parsed label/value pair (for meter ID tracking, etc.).
	* @param label The parsed label.
	* @param value The parsed value.
	*/
	virtual void handle_label_value(const std::string&, const std::string&) = 0;

	/**
	* @brief Construct a unique object ID for a given label, using meter ID.
	* @param label The label to use in the object ID.
	* @return Object ID string.
	*/
	std::string get_object_id(const std::string& label) const {
		std::string safe_label = sanitize_label(label);
		return std::string(MQTT_ID_BASE) + "_" + safe_label;
	}

	/**
	* @brief Construct the MQTT config topic for a given label, using meter ID.
	* @param label The label to use in the topic.
	* @return Config topic string, or empty if meter ID is not set.
	*/
	virtual std::string get_mqtt_config_topic(const std::string& label) const {
		if (!meter_id_.empty()) {
			std::string object_id = get_object_id(label);
			return std::string(MQTT_TOPIC_BASE) + "/" + meter_id_ + "/" + object_id + "/config";
		} else {
			return "";
		}
	}

	/**
	* @brief Get all Home Assistant discovery messages (topic, payload) for all labels.
	* @return Vector of (topic, payload) pairs.
	*/
	virtual std::vector<std::pair<std::string, std::string>> get_all_discovery_messages() const;

	/**
	* @brief Get the Home Assistant device class for a label.
	* @param label The label to check.
	* @return Device class string or nullptr if not applicable.
	*/
	virtual const char* get_ha_device_class(const std::string& label) const { return nullptr; }

	/**
	* @brief Get the Home Assistant state class for a label.
	* @param label The label to check.
	* @return State class string or nullptr if not applicable.
	*/
	virtual const char* get_ha_state_class(const std::string& label) const { return nullptr; }

	/**
	* @brief Get the Home Assistant unit for a label.
	* @param label The label to check.
	* @return Unit string or nullptr if not applicable.
	*/
	virtual const char* get_ha_unit(const std::string& label) const { return nullptr; }
		// Track if a frame is in progress (ADCO/ADSC seen)
	bool frame_in_progress_ = false;

	/**
	* @brief Should publish after this label? (default: only if ADCO/ADSC is received)
	*/
	virtual bool should_publish_frame(const std::string& label) const {
		// Only publish when ADCO/ADSC is received (frame start)
		return (label == "ADCO" || label == "ADSC");
	}
protected:
	std::string meter_id_;
};
