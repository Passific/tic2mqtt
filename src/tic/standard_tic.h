#pragma once
#include <set>
#include <string>

namespace mqtt {
    class async_client;
}
#include "tic_mode.h"

class StandardTIC : public TicMode {
public:
    const char* get_ha_device_class(const std::string& label) const override;
    const char* get_ha_state_class(const std::string& label) const override;
    const char* get_ha_unit(const std::string& label) const override;
    static constexpr unsigned long BAUDRATE = 9600;
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
    void handle_label_value(const std::string& label, const std::string& value);
};
