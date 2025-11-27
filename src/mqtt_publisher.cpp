#include "mqtt_publisher.h"
#include "tic/tic_utils.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <sstream>
#include <mqtt/async_client.h>


/**
 * @brief Construct a new MqttPublisher object.
 * @param server MQTT broker address.
 * @param client_id MQTT client ID.
 * @param user MQTT username.
 * @param pass MQTT password.
 * @param mode Reference to the TIC mode object.
 */
MqttPublisher::MqttPublisher(const std::string& server, const std::string& client_id, const std::string& user, const std::string& pass, const TicMode& mode)
	: server_(server), client_id_(client_id), user_(user), pass_(pass), mode_(mode), running_(false), mqtt_client_(nullptr), connected_(false) {}


/**
 * @brief Destroy the MqttPublisher object and disconnect from the broker.
 */
MqttPublisher::~MqttPublisher() {
	stop();
	if (mqtt_client_) {
		try {
			if (connected_) mqtt_client_->disconnect()->wait();
		} catch (...) {}
		delete mqtt_client_;
	}
}


/**
 * @brief Start the MQTT publisher thread.
 */
void MqttPublisher::start() {
	running_ = true;
	thread_ = std::thread(&MqttPublisher::run, this);
}


/**
 * @brief Stop the MQTT publisher thread and disconnect.
 */
void MqttPublisher::stop() {
	running_ = false;
	cv_.notify_all();
	if (thread_.joinable()) thread_.join();
}


/**
 * @brief Queue a label/value pair for publishing to MQTT.
 * @param label The label to publish.
 * @param value The value to publish.
 */
void MqttPublisher::publish_label_value(const std::string& label, const std::string& value) {
	// Queue the message for the publisher thread
	{
		std::lock_guard<std::mutex> lock(queue_mutex_);
		message_queue_.emplace(label, value);
	}
	cv_.notify_one();
}


/**
 * @brief Republish Home Assistant discovery messages for all known labels.
 */
void MqttPublisher::send_discovery() {
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
			pubmsg->set_retained(true);
			mqtt_client_->publish(pubmsg)->wait();
			std::cout << "[MQTT] Discovery published (retained): " << topic << std::endl;
		} catch (const mqtt::exception& exc) {
			std::cerr << "[MQTT] Discovery publish failed: " << exc.what() << std::endl;
		}
	}
}


/**
 * @brief Main thread loop for MQTT publishing and connection management.
 */
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
			// Publish Home Assistant discovery/config topics at startup
			send_discovery();
			break;
		} catch (const mqtt::exception& exc) {
			std::cerr << "[MQTT] Connection failed: " << exc.what() << ". Retrying in 5s..." << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(5));
		}
	}
	if (!connected_) return;

	while (running_) {
		std::unique_lock<std::mutex> lock(queue_mutex_);
		cv_.wait_for(lock, std::chrono::milliseconds(100), [this] { return !message_queue_.empty() || !running_; });
		bool should_publish = false;
		while (!message_queue_.empty()) {
			// Just pop all label/value pairs, we only care about the latest state
			message_queue_.pop();
			should_publish = true;
		}
		lock.unlock();

		// Only publish if meter_id is set (non-empty) and we received at least one label
		if (should_publish && !mode_.get_meter_id().empty()) {
			// Build JSON object from all label/values
			const auto& label_values = mode_.get_label_values();
			std::ostringstream oss;
			oss << "{";
			bool first = true;
			for (const auto& kv : label_values) {
				if (!first) oss << ",";
				first = false;
				oss << "\"" << sanitize_label(kv.first) << "\": {\"raw\": \"" << sanitize_value(kv.second.value) << "\"";
				if (!kv.second.timestamp.empty()) {
					oss << ", \"timestamp\": \"" << kv.second.timestamp << "\"";
				}
				oss << "}";
			}
			oss << "}";
			std::string topic = std::string("tic2mqtt/") + mode_.get_meter_id();
			std::string payload = oss.str();
			try {
				mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
				pubmsg->set_qos(1);
				mqtt_client_->publish(pubmsg)->wait();
				std::cout << "[MQTT] Published: " << topic << " = " << payload << std::endl;
			} catch (const mqtt::exception& exc) {
				std::cerr << "[MQTT] Publish failed: " << exc.what() << std::endl;
			}
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
