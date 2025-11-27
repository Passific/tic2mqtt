#pragma once

#include "tic_mode.h"

#include <set>
#include <string>

namespace mqtt {
	class async_client;
}

/**
 * @brief StandardTIC implements TIC mode for "standard" teleinfo frames.
 *        Provides label set, Home Assistant helpers, and meter ID tracking.
 */
class StandardTIC : public TicMode {
	const char* get_mode_name() const override { return "standard"; }
public:
	/**
	 * @brief Get the Home Assistant device class for a label (Standard TIC).
	 * @param label The label to check.
	 * @return Device class string or nullptr if not applicable.
	 */
	const char* get_ha_device_class(const std::string& label) const override;

	/**
	 * @brief Get the Home Assistant state class for a label (Standard TIC).
	 * @param label The label to check.
	 * @return State class string or nullptr if not applicable.
	 */
	const char* get_ha_state_class(const std::string& label) const override;

	/**
	 * @brief Get the Home Assistant unit for a label (Standard TIC).
	 * @param label The label to check.
	 * @return Unit string or nullptr if not applicable.
	 */
	const char* get_ha_unit(const std::string& label) const override;

	/**
	 * @brief Baudrate for standard TIC mode.
	 */
	static constexpr unsigned long BAUDRATE = 9600;

	/**
	 * @brief Get the set of all supported labels for standard TIC.
	 * @return Reference to the set of label strings.
	 */
	const std::set<std::string>& labels() const override {
		static const std::set<std::string> labels_set = {
			"ADSC", "CCAIN", "CCAIN-1", "CCASN", "CCASN-1", "DATE", "DPM1", "DPM2", "DPM3", "EAIT",
			"EASD01", "EASD02", "EASD03", "EASD04", "EASF01", "EASF02", "EASF03", "EASF04", "EASF05",
			"EASF06", "EASF07", "EASF08", "EASF09", "EASF10", "EAST", "ERQ1", "ERQ2", "ERQ3", "ERQ4",
			"FPM1", "FPM2", "FPM3", "IRMS1", "IRMS2", "IRMS3", "LTARF", "MSG1", "MSG2", "NGTF", "NJOURF",
			"NJOURF+1", "NTARF", "PCOUP", "PJOURF+1", "PPOINTE", "PREF", "PRM", "RELAIS", "SINSTI",
			"SINSTS", "SINSTS1", "SINSTS2", "SINSTS3", "SMAXIN", "SMAXIN-1", "SMAXSN", "SMAXSN-1",
			"SMAXSN1", "SMAXSN1-1", "SMAXSN2", "SMAXSN2-1", "SMAXSN3", "SMAXSN3-1", "STGE", "UMOY1",
			"UMOY2", "UMOY3", "URMS1", "URMS2", "URMS3", "VTIC"
		};
		return labels_set;
	}

	/**
	 * @brief Handle a parsed label/value pair for Standard TIC mode.
	 *        Sets the meter ID if the ADSC label is found.
	 * @param label The parsed label.
	 * @param value The parsed value.
	 */
	void handle_label_value(const std::string& label, const std::string& value) override;
};
