#include "gpio_handler.hpp"
#include <iostream>
#include <stdexcept>
#include <sstream>
#include "semaphores.hpp" // TODO

// Default constructor
GPIOHandler::GPIOHandler()
    : pin_(-1), is_input_(false), pull_up_(false),
      debounce_time_(50), callback_(nullptr), chip_(GPIO_CHIP_PATH) {}

// Parameterized constructor
GPIOHandler::GPIOHandler(int pin, bool is_input, bool pull_up,
                         std::chrono::milliseconds debounce,
                         Callback callback)
    : pin_(pin), is_input_(is_input), pull_up_(pull_up),
      debounce_time_(debounce), callback_(std::move(callback)),
      chip_(GPIO_CHIP_PATH)
{
    setup(pin, is_input, pull_up, debounce, callback_);
}

/**
 * @brief Destructor for GPIOHandler.
 */
GPIOHandler::~GPIOHandler()
{
    try {
        std::cerr << "[DEBUG] Destructor called for GPIOHandler pin " << pin_ << ".\n";
        stopMonitoring();
        cleanupPin();
    } catch (const std::exception &e) {
        std::cerr << "[ERROR] Exception in ~GPIOHandler(): " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception in ~GPIOHandler().\n";
    }
}

// GPIO setup method
void GPIOHandler::setup(int pin, bool is_input, bool pull_up,
                        std::chrono::milliseconds debounce, Callback callback)
{
    pin_ = pin;
    is_input_ = is_input;
    pull_up_ = pull_up;
    debounce_time_ = debounce;
    callback_ = std::move(callback);

    configurePin();
}

/**
 * @brief Configures the GPIO pin for input or output.
 */
void GPIOHandler::configurePin()
{
    std::lock_guard<std::mutex> lock(gpio_mutex_);

    // Ensure the pin is valid
    if (pin_ < 0) {
        throw std::runtime_error("Invalid GPIO pin number.");
    }

    // Attempt to get the GPIO line
    line_ = chip_.get_line(pin_);
    if (!line_) {
        throw std::runtime_error("Failed to get GPIO line for pin " + std::to_string(pin_));
    }

    // Release if already requested
    if (line_.is_requested()) {
        line_.release();
        if (line_.is_requested()) {
            throw std::runtime_error("Failed to release previously requested GPIO line for pin " + std::to_string(pin_));
        }
    }

    // Configure request
    gpiod::line_request request;
    request.consumer = "GPIOHandler";

    if (is_input_) {
        request.request_type = gpiod::line_request::EVENT_BOTH_EDGES;
        request.flags = pull_up_ ? gpiod::line_request::FLAG_BIAS_PULL_UP
                                 : gpiod::line_request::FLAG_BIAS_PULL_DOWN;
    } else {
        request.request_type = gpiod::line_request::DIRECTION_OUTPUT;
    }

    // Request the GPIO line
    line_.request(request);

    // Verify the request succeeded
    if (!line_.is_requested()) {
        throw std::runtime_error("Failed to request GPIO line for pin " + std::to_string(pin_));
    }
}

/**
 * @brief Starts monitoring the GPIO pin for edge events.
 */
void GPIOHandler::startMonitoring()
{
    if (!is_input_)
        return;

    running_ = true;
    while (running_ && !exit_wspr_loop.load() && !signal_shutdown.load())
    {
        try
        {
            if (!line_.is_requested())
                throw std::runtime_error("GPIO line not requested for pin " + std::to_string(pin_));

            if (line_.event_wait(std::chrono::seconds(1)))
            {
                auto event = line_.event_read();
                if (callback_)
                {
                    EdgeType edge = (event.event_type == gpiod::line_event::RISING_EDGE) ? EdgeType::RISING : EdgeType::FALLING;
                    callback_(edge, line_.get_value());
                }
            }
        }
        catch (const std::system_error &e)
        {
            if (e.code() == std::errc::invalid_argument)
                break;
            throw;
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("GPIO monitoring failed for pin " + std::to_string(pin_) + ": " + e.what());
        }
    }
}

/**
 * @brief Stops monitoring the GPIO pin.
 */
void GPIOHandler::stopMonitoring()
{
    if (!running_.exchange(false)) return;  // Ensure single shutdown
    std::cerr << "[DEBUG] Stopping monitoring for GPIO pin " << pin_ << ".\n";
}

/**
 * @brief Cleans up the GPIO pin, releasing resources.
 */
void GPIOHandler::cleanupPin()
{
    std::lock_guard<std::mutex> lock(gpio_mutex_);
    try {
        if (line_.is_requested()) {
            std::cerr << "[DEBUG] Releasing GPIO pin " << pin_ << ".\n";
            line_.release();
        } else {
            std::cerr << "[DEBUG] GPIO pin " << pin_ << " already released.\n";
        }
    } catch (const std::exception &e) {
        std::cerr << "[ERROR] Exception during cleanupPin(): " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception during cleanupPin().\n";
    }
}

/**
 * @brief Sets the GPIO output state.
 */
void GPIOHandler::setOutput(bool state)
{
    if (is_input_)
        throw std::runtime_error("Cannot set value on input pin " + std::to_string(pin_));

    line_.set_value(state ? 1 : 0);
}

/**
 * @brief Gets the current input state.
 */
bool GPIOHandler::getInputState() const
{
    if (!is_input_)
        throw std::runtime_error("Cannot read value from output pin " + std::to_string(pin_));

    return line_.get_value();
}
