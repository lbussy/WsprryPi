/**
 * @file main.cpp
 * @brief Entry point for the SignalHandler test application.
 *
 * This program sets up a global SignalHandler instance and assigns a custom
 * callback function to handle signals. It then blocks signals, registers the
 * callback, and waits for shutdown signals.
 */

#include "signal_handler.hpp"
#include <iostream>

/// Global unique pointer to the SignalHandler instance.
std::unique_ptr<SignalHandler> handler = nullptr;

/**
 * @brief Custom signal handling function.
 *
 * This function is called when a signal is received. It logs the signal and,
 * if critical, terminates immediately. Otherwise, it initiates a graceful shutdown.
 *
 * @param signum The signal number received.
 * @param is_critical Indicates whether the signal is critical.
 */
void custom_signal_handler(int signum, bool is_critical)
{
    std::string_view signal_name = SignalHandler::signal_to_string(signum);
    std::cout << "[DEBUG] Callback executed for signal " << signal_name << std::endl;

    if (is_critical)
    {
        std::cerr << "[ERROR] Critical signal received: " << signal_name << ". Performing immediate shutdown." << std::endl;
        std::quick_exit(signum);
    }
    else
    {
        std::cout << "[INFO] Intercepted signal: " << signal_name << ". Shutdown will proceed." << std::endl;

        if (handler) // Ensure handler is valid
        {
            SignalHandlerStatus status = handler->request_shutdown();

            if (status == SignalHandlerStatus::ALREADY_STOPPED)
            {
                std::cerr << "[WARN] Shutdown already in progress. Ignoring duplicate request." << std::endl;
            }
        }
        else
        {
            std::cerr << "[ERROR] Handler is null. Cannot request shutdown." << std::endl;
        }
    }
}

/**
 * @brief Main function.
 *
 * Initializes the SignalHandler instance, blocks signals, sets up the callback,
 * and waits for shutdown signals.
 *
 * @return int Returns 0 on successful shutdown, or 1 on failure.
 */
int main()
{
    handler = std::make_unique<SignalHandler>();

    if (handler->block_signals() != SignalHandlerStatus::SUCCESS)
    {
        std::cerr << "[ERROR] Failed to block signals. Exiting." << std::endl;
        return 1;
    }

    if (handler->set_callback(custom_signal_handler) != SignalHandlerStatus::SUCCESS)
    {
        std::cerr << "[ERROR] Failed to set signal callback." << std::endl;
        return 1;
    }

    // Wait indefinitely for a shutdown signal
    handler->wait_for_shutdown();

    // Example: Wait for 5 seconds before proceeding
    // if (handler->wait_for_shutdown(5) != SignalHandlerStatus::SUCCESS)
    // {
    //     std::cerr << "[ERROR] Shutdown process did not complete as expected." << std::endl;
    //     handler->request_shutdown();
    //     handler->wait_for_shutdown(1);
    // }

    handler->stop();
    handler.reset();

    return 0;
}
