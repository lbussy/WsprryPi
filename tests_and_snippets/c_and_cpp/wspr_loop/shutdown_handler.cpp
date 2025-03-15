// TODO:  Check Doxygen

// Primary header for this source file
#include "shutdown_handler.hpp"

// Project headers
#include "gpio_handler.hpp"
#include "scheduling.hpp"
#include "arg_parser.hpp"
#include "logging.hpp"

// Standard library headers
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

// System headers
#include <signal.h>
#include <termios.h>
#include <unistd.h>

std::atomic<bool> shutdown_in_progress(false);
std::atomic<bool> signal_shutdown{false};

/**
 * @brief Creates or reinitializes the shutdown_pin GPIO input.
 * @param pin The GPIO pin number (default: 19).
 */
void enable_shutdown_pin(int pin)
{
    // If already active, release and reconfigure.
    if (shutdown_handler)
    {
        shutdown_handler.reset();

        if (button_thread.joinable())
        {
            llog.logS(DEBUG, "Closing button monitor threads.");
            button_thread.join();
        }
    }

    shutdown_pin_number = pin;
    shutdown_handler = std::make_unique<GPIOHandler>(shutdown_pin_number, true, true, std::chrono::milliseconds(50), shutdown_system);
    button_thread = std::thread(&GPIOHandler::startMonitoring, shutdown_handler.get());
    llog.logS(INFO, "Shutdown button enabled on GPIO", shutdown_pin_number, ".");
}

/**
 * @brief Disables the shutdown_pin GPIO.
 */
void disable_shutdown_pin()
{
    if (shutdown_handler)
    {
        shutdown_handler.reset();
        llog.logS(INFO, "Releasing existing shutdown pin (GPIO", shutdown_pin_number, ")");
    }

    if (button_thread.joinable())
    {
        llog.logS(DEBUG, "Closing button monitor threads.");
        button_thread.join();
    }
}

/**
 * @brief Shuts down the system after a 3-second delay.
 * @details This function checks if the user has root privileges and, if so,
 *          executes a shutdown command with a 3-second delay.
 *
 * @throws std::runtime_error If the user lacks root privileges or the shutdown
 *                            command fails to execute.
 *
 * @note This function requires root access to execute the shutdown command.
 *       Ensure the program runs with elevated privileges (e.g., using `sudo`).
 */
void shutdown_system(GPIOHandler::EdgeType edge, bool state)
{
    if (edge == GPIOHandler::EdgeType::FALLING)
    {
        // Execute the shutdown command with a 5-second delay.
        int result = std::system("sleep 5 && shutdown -h now &");

        // Check if the shutdown command was successful.
        if (result != 0)
        {
            throw std::runtime_error("Failed to execute shutdown command.");
            return;
        }
        else
        {
            llog.logS(INFO, "Shutdown triggered by shutdown button.");
            // Set shutdown flag and notify waiting threads.
            shutdown_in_progress.store(true);
            exit_wspr_loop.store(true); // Set exit flag
            shutdown_cv.notify_all();            // Wake up waiting threads
        }
    }
}
