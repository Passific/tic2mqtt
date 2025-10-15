#pragma once
#include <set>
#include <string>

namespace mqtt {
	class async_client;
}
#include "tic_mode.h"

/**
 * @brief HistoriqueTIC implements TIC mode for "historique" teleinfo frames.
 *        Provides label set, Home Assistant helpers, and meter ID tracking.
 */
class HistoriqueTIC : public TicMode {
public:
	/**
	 * @brief Get the Home Assistant device class for a label (Historique TIC).
	 * @param label The label to check.
	 * @return Device class string or nullptr if not applicable.
	 */
	const char* get_ha_device_class(const std::string& label) const override;

	/**
	 * @brief Get the Home Assistant state class for a label (Historique TIC).
	 * @param label The label to check.
	 * @return State class string or nullptr if not applicable.
	 */
	const char* get_ha_state_class(const std::string& label) const override;

	/**
	 * @brief Get the Home Assistant unit for a label (Historique TIC).
	 * @param label The label to check.
	 * @return Unit string or nullptr if not applicable.
	 */
	const char* get_ha_unit(const std::string& label) const override;

	/**
	 * @brief Baudrate for historique TIC mode.
	 */
	static constexpr unsigned long BAUDRATE = 1200;

	/**
	 * @brief Get the set of all supported labels for historique TIC.
	 * @return Reference to the set of label strings.
	 */
	const std::set<std::string>& labels() const override {
		static const std::set<std::string> labels_set = {
			"ADCO", "ADIR1", "ADIR2", "ADIR3", "ADPS", "BASE", "BBRHCJB", "BBRHCJR", "BBRHCJW",
			"BBRHPJB", "BBRHPJR", "BBRHPJW", "DEMAIN", "EJPHN", "EJPHPM", "HCHC", "HCHP", "HHPHC",
			"IINST", "IINST1", "IINST2", "IINST3", "IMAX", "IMAX1", "IMAX2", "IMAX3", "ISOUSC",
			"MOTDETAT", "OPTARIF", "PAPP", "PEJP", "PMAX", "PPOT", "PTEC"
		};
		return labels_set;
	}

	/**
	 * @brief Handle a parsed label/value pair for Historique TIC mode.
	 *        Sets the meter ID if the ADCO label is found.
	 * @param label The parsed label.
	 * @param value The parsed value.
	 */
	void handle_label_value(const std::string& label, const std::string& value);
};
