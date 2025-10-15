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

/**
 * @brief Publishes TIC label/value pairs to an MQTT broker, including Home Assistant discovery.
 */
class MqttPublisher {
public:
	/**
	 * @brief Construct a new MqttPublisher object.
	 * @param server MQTT broker address.
	 * @param client_id MQTT client ID.
	 * @param user MQTT username.
	 * @param pass MQTT password.
	 * @param mode Reference to the TIC mode object.
	 */
	MqttPublisher(const std::string& server, const std::string& client_id, const std::string& user, const std::string& pass, const TicMode& mode);

	/**
	 * @brief Destroy the MqttPublisher object and disconnect from the broker.
	 */
	~MqttPublisher();

	/**
	 * @brief Start the MQTT publisher thread.
	 */
	void start();

	/**
	 * @brief Stop the MQTT publisher thread and disconnect.
	 */
	void stop();

	/**
	 * @brief Queue a label/value pair for publishing to MQTT.
	 * @param label The label to publish.
	 * @param value The value to publish.
	 */
	void publish_label_value(const std::string& label, const std::string& value);

	/**
	 * @brief Republish Home Assistant discovery messages for all known labels.
	 */
	void resend_discovery();
private:
	/**
	 * @brief Main thread loop for MQTT publishing and connection management.
	 */
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
