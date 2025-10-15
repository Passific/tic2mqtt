#include "serial_reader.h"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <iostream>
#include <cstring>

SerialReader::SerialReader(const std::string& port, unsigned long baudrate)
    : port_(port), baudrate_(baudrate), running_(false) {}

SerialReader::~SerialReader() {
    stop();
}

void SerialReader::start() {
    running_ = true;
    thread_ = std::thread(&SerialReader::run, this);
}

void SerialReader::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

bool SerialReader::pop_line(std::string& line) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty()) return false;
    line = queue_.front();
    queue_.pop();
    return true;
}

void SerialReader::run() {
    while (running_) {
        int fd = -1;
        while (running_) {
            fd = open(port_.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
            if (fd < 0) {
                std::cerr << "[Serial] Open failed: " << strerror(errno) << ". Retrying in 5s..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            // Configure port
            struct termios tty;
            if (tcgetattr(fd, &tty) != 0) {
                std::cerr << "[Serial] tcgetattr failed: " << strerror(errno) << std::endl;
                close(fd);
                fd = -1;
                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }
            cfsetospeed(&tty, baudrate_);
            cfsetispeed(&tty, baudrate_);
            tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS7; // 7 data bits
            tty.c_cflag |= PARENB; // Even parity
            tty.c_cflag &= ~PARODD;
            tty.c_cflag &= ~CSTOPB; // 1 stop bit
            tty.c_cflag |= CLOCAL | CREAD;
            tty.c_iflag = IGNPAR;
            tty.c_oflag = 0;
            tty.c_lflag = 0;
            tty.c_cc[VMIN] = 1;
            tty.c_cc[VTIME] = 10; // 1s timeout
            if (tcsetattr(fd, TCSANOW, &tty) != 0) {
                std::cerr << "[Serial] tcsetattr failed: " << strerror(errno) << std::endl;
                close(fd);
                fd = -1;
                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }
            break;
        }
        if (fd < 0) continue;
        std::string buffer;
        char ch;
        while (running_) {
            ssize_t n = read(fd, &ch, 1);
            if (n == 1) {
                if (ch == '\n') {
                    if (!buffer.empty()) {
                        std::lock_guard<std::mutex> lock(mutex_);
                        queue_.push(buffer);
                        cv_.notify_one();
                        buffer.clear();
                    }
                } else if (ch != '\r') {
                    buffer += ch;
                }
            } else if (n < 0) {
                std::cerr << "[Serial] Read error: " << strerror(errno) << ". Reopening port..." << std::endl;
                close(fd);
                std::this_thread::sleep_for(std::chrono::seconds(2));
                break;
            }
        }
        if (fd >= 0) close(fd);
    }
}
