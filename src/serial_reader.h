#pragma once
#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

class SerialReader {
public:
    SerialReader(const std::string& port, unsigned long baudrate);
    ~SerialReader();
    void start();
    void stop();
    bool pop_line(std::string& line);
private:
    void run();
    std::string port_;
    unsigned long baudrate_;
    std::thread thread_;
    std::queue<std::string> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool running_ = false;
};
