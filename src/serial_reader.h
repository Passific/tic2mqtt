#pragma once
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

/**
 * @brief Reads lines from a serial port asynchronously and queues them for processing.
 */
class SerialReader {
public:
	/**
	 * @brief Construct a new SerialReader object.
	 * @param port Serial port device path.
	 * @param baudrate Serial port baudrate.
	 */
	SerialReader(const std::string& port, unsigned long baudrate);

	/**
	 * @brief Destroy the SerialReader object and stop the thread.
	 */
	~SerialReader();

	/**
	 * @brief Start the serial reader thread.
	 */
	void start();

	/**
	 * @brief Stop the serial reader thread.
	 */
	void stop();

	/**
	 * @brief Pop a line from the serial input queue.
	 * @param line Output: next line from the queue.
	 * @return true if a line was available, false otherwise.
	 */
	bool pop_line(std::string& line);
private:
	/**
	 * @brief Main thread loop for reading from the serial port and queuing lines.
	 */
	void run();

	std::string port_;
	unsigned long baudrate_;
	std::thread thread_;
	std::queue<std::string> queue_;
	std::mutex mutex_;
	std::condition_variable cv_;
	bool running_ = false;
};
