#pragma once
#include <set>
#include <string>

namespace mqtt {
    class async_client;
}
#include "tic_mode.h"

class HistoriqueTIC : public TicMode {
public:
    const char* get_ha_device_class(const std::string& label) const override;
    const char* get_ha_state_class(const std::string& label) const override;
    const char* get_ha_unit(const std::string& label) const override;
    static constexpr unsigned long BAUDRATE = 1200;
    const std::set<std::string>& labels() const override {
        static const std::set<std::string> labels_set = {
            "ADCO", "ADIR1", "ADIR2", "ADIR3", "ADPS", "BASE", "BBRHCJB", "BBRHCJR", "BBRHCJW",
            "BBRHPJB", "BBRHPJR", "BBRHPJW", "DEMAIN", "EJPHN", "EJPHPM", "HCHC", "HCHP", "HHPHC",
            "IINST", "IINST1", "IINST2", "IINST3", "IMAX", "IMAX1", "IMAX2", "IMAX3", "ISOUSC",
            "MOTDETAT", "OPTARIF", "PAPP", "PEJP", "PMAX", "PPOT", "PTEC"
        };
        return labels_set;
    }
    void handle_label_value(const std::string& label, const std::string& value);
};
