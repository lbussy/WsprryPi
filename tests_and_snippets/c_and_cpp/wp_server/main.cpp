#ifdef DEBUG_MAIN_WPSERVER

#include <atomic>      // For std::atomic
#include <iostream>    // For std::cout, std::endl
#include <csignal>     // For std::signal, SIGINT, SIGTERM
#include <chrono>      // For std::chrono
#include <thread>      // For std::this_thread::sleep_for
#include <string>      // For std::string
#include "wp_server.hpp"  // Include WsprryPi_Server definition
#include "lcblog.hpp"     // Include LCBLog logging class

std::atomic<bool> running(true);  // Atomic flag for server control

LCBLog llog;

void signal_handler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down server..." << std::endl;
    running = false;
}

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    WsprryPi_Server server(31415, llog);  // Explicitly pass logger
    server.start();

    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Stopping server..." << std::endl;
    server.stop();
    std::cout << "Server stopped." << std::endl;

    return 0;
}

#endif // DEBUG_MAIN_WPSERVER