#include <atomic>      // For std::atomic
#include <iostream>    // For std::cout, std::endl
#include <csignal>     // For std::signal, SIGINT, SIGTERM
#include <chrono>      // For std::chrono
#include <thread>      // For std::this_thread::sleep_for
#include <string>      // For std::string
#include "wp_server.hpp"  // Include WsprryPi_Server definition
#include "lcblog.hpp"     // Include LCBLog logging class

std::atomic<bool> main_running(true);  // Atomic flag for server control

constexpr int port = 31415;

LCBLog llog;
WsprryPi_Server server(port, llog);  // Global server instance (initialized after llog)

void signal_handler(int signum) {
    llog.logS(INFO, "Received signal:", signum);
    main_running = false;
}

int main() {
    llog.setLogLevel(DEBUG);
    llog.enableTimestamps(true);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    server.start();

    while (main_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    llog.logS(INFO, "Stopping server.");
    server.stop();

    return 0;
}
