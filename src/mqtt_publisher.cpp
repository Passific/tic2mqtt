#include "mqtt_publisher.h"
#include "tic/tic_utils.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <sstream>
#include <mqtt/async_client.h>


MqttPublisher::MqttPublisher(const std::string& server, const std::string& client_id, const std::string& user, const std::string& pass, const TicMode& mode)
    : server_(server), client_id_(client_id), user_(user), pass_(pass), mode_(mode), running_(false), mqtt_client_(nullptr), connected_(false) {}


MqttPublisher::~MqttPublisher() {
    stop();
    if (mqtt_client_) {
        try {
            if (connected_) mqtt_client_->disconnect()->wait();
        } catch (...) {}
        delete mqtt_client_;
    }
}


void MqttPublisher::start() {
    running_ = true;
    thread_ = std::thread(&MqttPublisher::run, this);
}


void MqttPublisher::stop() {
    running_ = false;
    cv_.notify_all();
    if (thread_.joinable()) thread_.join();
}


void MqttPublisher::publish_label_value(const std::string& label, const std::string& value) {
    // Queue the message for the publisher thread
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        message_queue_.emplace(label, value);
    }
    cv_.notify_one();
}


void MqttPublisher::resend_discovery() {
    // Republish Home Assistant discovery messages for all known labels
    if (!connected_ || !mqtt_client_) {
        std::cerr << "[MQTT] Not connected, cannot resend discovery." << std::endl;
        return;
    }
    std::vector<std::pair<std::string, std::string>> discovery_msgs = mode_.get_all_discovery_messages();
    for (size_t i = 0; i < discovery_msgs.size(); ++i) {
        const std::string& topic = discovery_msgs[i].first;
        const std::string& payload = discovery_msgs[i].second;
        try {
            mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
            pubmsg->set_qos(1);
            mqtt_client_->publish(pubmsg)->wait();
            std::cout << "[MQTT] Discovery republished: " << topic << std::endl;
        } catch (const mqtt::exception& exc) {
            std::cerr << "[MQTT] Discovery publish failed: " << exc.what() << std::endl;
        }
    }
}


void MqttPublisher::run() {
    // Connect to MQTT broker


    mqtt_client_ = new mqtt::async_client(server_, client_id_);
    mqtt::connect_options connOpts;
    if (!user_.empty()) {
        connOpts.set_user_name(user_);
        connOpts.set_password(pass_);
    }
    while (running_) {
        try {
            auto tok = mqtt_client_->connect(connOpts);
            tok->wait();
            connected_ = true;
            std::cout << "[MQTT] Connected to broker: " << server_ << std::endl;
            break;
        } catch (const mqtt::exception& exc) {
            std::cerr << "[MQTT] Connection failed: " << exc.what() << ". Retrying in 5s..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    if (!connected_) return;

    // Subscribe to Home Assistant status topic
    const std::string ha_status_topic = "homeassistant/status";
    try {
        mqtt_client_->start_consuming();
        mqtt_client_->subscribe(ha_status_topic, 1)->wait();
        std::cout << "[MQTT] Subscribed to HA status topic: " << ha_status_topic << std::endl;
    } catch (const mqtt::exception& exc) {
        std::cerr << "[MQTT] Failed to subscribe to HA status: " << exc.what() << std::endl;
    }


    while (running_) {
        // Check for HA status messages
    mqtt::const_message_ptr msg = mqtt_client_->try_consume_message_for(std::chrono::milliseconds(10));
        if (msg && msg->get_topic() == ha_status_topic) {
            std::string payload = msg->to_string();
            if (payload == "online") {
                std::cout << "[MQTT] HA status is online, resending discovery." << std::endl;
                resend_discovery();
            }
        }

        std::unique_lock<std::mutex> lock(queue_mutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(100), [this] { return !message_queue_.empty() || !running_; });
        while (!message_queue_.empty()) {
            std::pair<std::string, std::string> label_value = message_queue_.front();
            std::string label = label_value.first;
            std::string value = label_value.second;
            message_queue_.pop();
            lock.unlock();

            // Compose topic and payload
            std::string topic = mode_.get_mqtt_topic(label);
            std::string payload = value;
            try {
                mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
                pubmsg->set_qos(1);
                mqtt_client_->publish(pubmsg)->wait();
                std::cout << "[MQTT] Published: " << topic << " = " << value << std::endl;
            } catch (const mqtt::exception& exc) {
                std::cerr << "[MQTT] Publish failed: " << exc.what() << std::endl;
            }

            lock.lock();
        }
    }

    // Disconnect
    try {
        mqtt_client_->disconnect()->wait();
        connected_ = false;
        std::cout << "[MQTT] Disconnected from broker" << std::endl;
    } catch (const mqtt::exception& exc) {
        std::cerr << "[MQTT] Disconnect failed: " << exc.what() << std::endl;
    }
}
