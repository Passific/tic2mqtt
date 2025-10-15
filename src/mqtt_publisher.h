#pragma once
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <set>
#include <map>
#include "tic_mode.h"

namespace mqtt {
    class async_client;
}

class MqttPublisher {
public:
    MqttPublisher(const std::string& server, const std::string& client_id, const std::string& user, const std::string& pass, const TicMode& mode);
    ~MqttPublisher();
    void start();
    void stop();
    void publish_label_value(const std::string& label, const std::string& value);
    void resend_discovery();
private:
    void run();
    std::string server_;
    std::string client_id_;
    std::string user_;
    std::string pass_;
    const TicMode& mode_;
    std::thread thread_;
    bool running_ = false;
    std::set<std::string> published_labels_;
    std::map<std::string, std::string> latest_values_;

    // MQTT connection
    mqtt::async_client* mqtt_client_ = nullptr;
    bool connected_ = false;

    // For async publishing
    std::queue<std::pair<std::string, std::string>> message_queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
};
