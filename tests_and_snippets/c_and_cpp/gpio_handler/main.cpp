// main.cpp
#include "gpio_handler.hpp"
#include "semaphores.hpp"
#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>

std::atomic<bool> shutdown_triggered{false};

void clean_shutdown()
{
    static std::once_flag shutdown_flag;
    std::call_once(shutdown_flag, []() {
        std::cerr << "[DEBUG] Shutdown complete. Exiting now.\n";

        exit_wspr_loop.store(true);
        signal_shutdown.store(true);
        shutdown_triggered.store(true, std::memory_order_release);

        std::cerr << "[DEBUG] Cleaning up GPIO handlers.\n";
        shutdown_handler.reset();
        led_handler.reset();

        if (button_thread.joinable()) {
            std::cerr << "[DEBUG] Joining button_thread.\n";
            button_thread.join();
        }

        if (led_thread.joinable()) {
            std::cerr << "[DEBUG] Joining led_thread.\n";
            led_thread.join();
        }

        std::cerr << "[DEBUG] All threads stopped. Exiting.\n";
    });
}

void shutdown_system(GPIOHandler::EdgeType edge, bool state)
{
    if (edge == GPIOHandler::EdgeType::FALLING && !state)
    {
        std::cerr << "[DEBUG] Shutdown signal received. Flashing LED and shutting down system.\n";

        for (int i = 0; i < 3; ++i)
        {
            led_handler->setOutput(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            led_handler->setOutput(false);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        try {
            clean_shutdown();
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception during clean shutdown: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[ERROR] Unknown exception during clean shutdown.\n";
        }
    }
}

int main()
{
    std::cerr.setf(std::ios::unitbuf); // Flush output immediately

    try
    {
        // Initialize GPIO handlers
        std::cerr << "[DEBUG] Initializing GPIO handlers.\n";

        // Create GPIOHandler instances
        //
        // LED as output (no monitoring needed)
        led_handler = std::make_unique<GPIOHandler>(LED_PIN, false, false);
        std::cerr << "[DEBUG] LED GPIO setup complete.\n";
        //
        // Shutdown button as input with callback
        shutdown_handler = std::make_unique<GPIOHandler>(
            SHUTDOWN_PIN, true, true, std::chrono::milliseconds(50), shutdown_system);
        std::cerr << "[DEBUG] Shutdown GPIO setup complete.\n";

        // Start monitoring the shutdown button
        std::thread button_thread;
        if (shutdown_handler)
        {
            std::cerr << "[DEBUG] Starting shutdown monitoring thread.\n";
            button_thread = std::thread(&GPIOHandler::startMonitoring, shutdown_handler.get());
        }

        while (!shutdown_triggered.load())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
        clean_shutdown();
    }
    catch (const std::exception &e) {
        std::cerr << "[ERROR] Exception in main(): " << e.what() << std::endl;
        clean_shutdown();
        return 1;
    }
    catch (...) {
        std::cerr << "[ERROR] Unknown exception in main().\n";
        clean_shutdown();
        return 1;
    }

    return 0;

    return 0;
}