
#include "historique_tic.h"
#include "tic_utils.h"
// Call this when a label/value is parsed
void HistoriqueTIC::handle_label_value(const std::string& label, const std::string& value) {
    if (label == "ADCO") {
        set_meter_id(sanitize_label(value));
    }
    // ...existing label handling logic...
}

const char* HistoriqueTIC::get_ha_device_class(const std::string& label) const {
    static const std::set<std::string> apparent_power = {
        "PAPP", "PREF", "PCOUP", "SINSTS", "SINSTS1", "SINSTS2", "SINSTS3"
    };
    static const std::set<std::string> current = {
        "ADIR1", "ADIR2", "ADIR3", "ADPS", "IINST", "IINST1", "IINST2", "IINST3",
        "IMAX", "IMAX1", "IMAX2", "IMAX3", "ISOUSC"
    };
    static const std::set<std::string> energy = {
        "BASE", "BBRHCJB", "BBRHCJR", "BBRHCJW", "BBRHPJB", "BBRHPJR", "BBRHPJW", "EJPHN",
        "EJPHPM", "HCHC", "HCHP"
    };
    static const std::set<std::string> voltage = {};
    static const std::set<std::string> power = {"PMAX"};
    static const std::set<std::string> reactive_energy = {};
    if (apparent_power.count(label)) return "apparent_power";
    if (current.count(label)) return "current";
    if (energy.count(label)) return "energy";
    if (voltage.count(label)) return "voltage";
    if (power.count(label)) return "power";
    if (reactive_energy.count(label)) return "reactive_energy";
    return nullptr;
}

const char* HistoriqueTIC::get_ha_state_class(const std::string& label) const {
    static const std::set<std::string> total_increasing = {
        "BASE", "BBRHCJB", "BBRHCJR", "BBRHCJW", "BBRHPJB", "BBRHPJR", "BBRHPJW", "EJPHN",
        "EJPHPM", "HCHC", "HCHP"
    };
    static const std::set<std::string> measurement = {
        "IINST", "IINST1", "IINST2", "IINST3", "PAPP"
    };
    if (total_increasing.count(label)) return "total_increasing";
    if (measurement.count(label)) return "measurement";
    return nullptr;
}

const char* HistoriqueTIC::get_ha_unit(const std::string& label) const {
    static const std::set<std::string> ampere = {
        "ADPS", "IINST", "IINST1", "IINST2", "IINST3", "IMAX", "IMAX1", "IMAX2", "IMAX3", "ISOUSC"
    };
    static const std::set<std::string> watt_hour = {
        "BASE", "BBRHCJB", "BBRHCJR", "BBRHCJW", "BBRHPJB", "BBRHPJR", "BBRHPJW", "EJPHN",
        "EJPHPM", "HCHC", "HCHP"
    };
    static const std::set<std::string> volt = {};
    static const std::set<std::string> volt_ampere = {"PAPP", "PREF", "PCOUP"};
    static const std::set<std::string> watt = {"PMAX"};
    static const std::set<std::string> varh = {};
    if (ampere.count(label)) return "A";
    if (watt_hour.count(label)) return "Wh";
    if (volt.count(label)) return "V";
    if (volt_ampere.count(label)) return "VA";
    if (watt.count(label)) return "W";
    if (varh.count(label)) return "varh";
    return nullptr;
}
