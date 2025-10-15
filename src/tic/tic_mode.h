#pragma once
#include "tic_utils.h"

#include <string>
#include <set>
#include <vector>
#include <utility>

namespace mqtt {
    class async_client;
}


class TicMode {
public:
    virtual ~TicMode() = default;
    virtual const std::set<std::string>& labels() const = 0;
    // Meter ID handling
    void set_meter_id(const std::string& id) { meter_id_ = id; }
    const std::string& get_meter_id() const { return meter_id_; }
    // Topic construction
    virtual std::string get_mqtt_topic(const std::string& label) const {
        if (!meter_id_.empty())
            return std::string(MQTT_TOPIC_BASE) + meter_id_ + "/" + sanitize_label(label) + "/state";
        else
            return std::string(MQTT_TOPIC_BASE) + sanitize_label(label) + "/state";
    }
    virtual void publish_ha_discovery(mqtt::async_client& client, const std::string& label) const;
    virtual std::vector<std::pair<std::string, std::string>> get_all_discovery_messages() const { return {}; }
    virtual const char* get_ha_device_class(const std::string& label) const { return nullptr; }
    virtual const char* get_ha_state_class(const std::string& label) const { return nullptr; }
    virtual const char* get_ha_unit(const std::string& label) const { return nullptr; }
protected:
    std::string meter_id_;
};
