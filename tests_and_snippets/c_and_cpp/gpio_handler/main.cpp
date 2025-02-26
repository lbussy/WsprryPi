// Primary header for this source file
// ...

// Project headers
#include "gpio_handler.hpp"

// Standard library headers
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

// System headers
// ...

std::atomic<bool> shutdown_triggered{false};

/**
 * @brief Cleanly shuts down the application.
 */
void clean_shutdown() {
    shutdown_triggered.store(true, std::memory_order_release);

    if (shutdown_handler) {
        std::cerr << "[DEBUG] Resetting shutdown handler.\n";
        shutdown_handler.reset();
    }

    if (led_handler) {
        std::cerr << "[DEBUG] Resetting LED handler.\n";
        led_handler.reset(); 
    }

    if (button_thread.joinable()) {
        std::cerr << "[DEBUG] Closing button monitor threads.\n";
        button_thread.join();
    }
    std::cerr << "[DEBUG] Thread shutdown complete.\n";
}

/**
 * @brief Callback for shutdown button press.
 */
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

        clean_shutdown();
    }
}

/**
 * @brief Main entry point for the application.
 */
int main()
{
    std::cerr.setf(std::ios::unitbuf); // Ensure immediate flushing of debug output.

    try
    {
        std::cerr << "[DEBUG] Initializing GPIO handlers.\n";

        // Initialize LED GPIO (output)
        led_handler = std::make_unique<GPIOHandler>(LED_PIN, false, false);
        std::cerr << "[DEBUG] LED GPIO setup complete.\n";

        // Initialize Shutdown GPIO (input with pull-up and callback)
        shutdown_handler = std::make_unique<GPIOHandler>(
            SHUTDOWN_PIN, true, true, std::chrono::milliseconds(50), shutdown_system);
        std::cerr << "[DEBUG] Shutdown GPIO setup complete.\n";

        // Start monitoring the shutdown button
        button_thread = std::thread(&GPIOHandler::startMonitoring, shutdown_handler.get());
        std::cerr << "[DEBUG] Starting shutdown monitoring thread.\n";

        while (!shutdown_triggered.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Prevent busy-waiting
        }

        // Trigger clean shutdown
        clean_shutdown();
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] Exception in main: " << e.what() << '\n';
        clean_shutdown();
        return 1;
    }

    return 0;
}
