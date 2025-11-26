
#include "standard_tic.h"
#include "tic_utils.h"

void StandardTIC::handle_label_value(const std::string& label, const std::string& value) {
	if (label == "ADSC") {
		// Publish previous frame if any
		if (!label_values_.empty()) {
			frame_in_progress_ = false;
			// Set a flag or call a callback if needed (handled in main loop)
		}
		set_meter_id(sanitize_label(value));
		label_values_.clear();
		frame_in_progress_ = true;
	}
	// Store all label/values
	label_values_[label] = value;
}

const char* StandardTIC::get_ha_device_class(const std::string& label) const {
	static const std::set<std::string> apparent_power = {
		"SINSTS", "SINSTS1", "SINSTS2", "SINSTS3",
		"SMAXSN", "SMAXSN1", "SMAXSN2", "SMAXSN3", "SMAXSN-1", "SMAXSN1-1", "SMAXSN2-1", "SMAXSN3-1",
		"SINSTI", "SMAXIN", "SMAXIN-1"
	};
	static const std::set<std::string> current = {
		"IRMS1", "IRMS2", "IRMS3"
	};
	static const std::set<std::string> energy = {
		"EAST", "EASF01", "EASF02", "EASF03", "EASF04", "EASF05", "EASF06", "EASF07", "EASF08", "EASF09", "EASF10", "EASD01", "EASD02", "EASD03", "EASD04", "EAIT"
	};
	static const std::set<std::string> voltage = {
		"URMS1", "URMS2", "URMS3", "UMOY1", "UMOY2", "UMOY3"
	};
	static const std::set<std::string> power = {
		"CCASN", "CCASN-1", "CCAIN", "CCAIN-1"
	};
	static const std::set<std::string> reactive_energy = {
		"ERQ1", "ERQ2", "ERQ3", "ERQ4"
	};
	if (apparent_power.count(label)) return "apparent_power";
	if (current.count(label)) return "current";
	if (energy.count(label)) return "energy";
	if (voltage.count(label)) return "voltage";
	if (power.count(label)) return "power";
	if (reactive_energy.count(label)) return "reactive_energy";
	return nullptr;
}

const char* StandardTIC::get_ha_state_class(const std::string& label) const {
	static const std::set<std::string> total_increasing = {
		"EAST", "EASF01", "EASF02", "EASF03", "EASF04", "EASF05", "EASF06", "EASF07", "EASF08", "EASF09", "EASF10", "EASD01", "EASD02", "EASD03", "EASD04", "EAIT"
	};
	static const std::set<std::string> measurement = {
		"IRMS1", "IRMS2", "IRMS3", "URMS1", "URMS2", "URMS3", "UMOY1", "UMOY2", "UMOY3", "SINSTS", "SINSTS1", "SINSTS2", "SINSTS3", "SINSTI"
	};
	if (total_increasing.count(label)) return "total_increasing";
	if (measurement.count(label)) return "measurement";
	return nullptr;
}

const char* StandardTIC::get_ha_unit(const std::string& label) const {
	static const std::set<std::string> ampere = {
		"IRMS1", "IRMS2", "IRMS3"
	};
	static const std::set<std::string> watt_hour = {
		"EAST", "EASF01", "EASF02", "EASF03", "EASF04", "EASF05", "EASF06", "EASF07", "EASF08", "EASF09", "EASF10", "EASD01", "EASD02", "EASD03", "EASD04", "EAIT"
	};
	static const std::set<std::string> volt = {
		"URMS1", "URMS2", "URMS3", "UMOY1", "UMOY2", "UMOY3"
	};
	static const std::set<std::string> volt_ampere = {
		"SINSTS", "SINSTS1", "SINSTS2", "SINSTS3",
		"SMAXSN", "SMAXSN1", "SMAXSN2", "SMAXSN3", "SMAXSN-1", "SMAXSN1-1", "SMAXSN2-1", "SMAXSN3-1",
		"SINSTI", "SMAXIN", "SMAXIN-1"
	};
	static const std::set<std::string> watt = {
		"CCASN", "CCASN-1", "CCAIN", "CCAIN-1"
	};
	static const std::set<std::string> varh = {
		"ERQ1", "ERQ2", "ERQ3", "ERQ4"
	};
	if (ampere.count(label)) return "A";
	if (watt_hour.count(label)) return "Wh";
	if (volt.count(label)) return "V";
	if (volt_ampere.count(label)) return "VA";
	if (watt.count(label)) return "W";
	if (varh.count(label)) return "varh";
	return nullptr;
}
