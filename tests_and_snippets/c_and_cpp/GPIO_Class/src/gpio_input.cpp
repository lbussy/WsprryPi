/**
 * @class GPIOInput
 * @brief Monitors a GPIO pin using libgpiod with thread-based event handling.
 *
 * This class allows for edge-triggered monitoring of a GPIO pin using the
 * libgpiod C++ API. It supports edge detection (rising or falling), optional
 * internal pull-up or pull-down configuration, CPU priority control, debounce
 * management, and thread-safe lifecycle operations.
 *
 * Designed for use on the Raspberry Pi platform with BCM GPIO numbering,
 * the class is thread-based and suitable for global instantiation.
 *
 * Example usage:
 * @code
 * GPIOInput monitor;
 * monitor.enable(19, false, GPIOInput::PullMode::PullUp, []() {
 *     std::cout << "Shutdown button pressed." << std::endl;
 * });
 * @endcode
 *
 * The monitoring thread will invoke the callback only once per edge trigger
 * until resetTrigger() is called. The thread can be stopped, reconfigured,
 * and restarted as needed.
 *
 * Dependencies:
 *  - libgpiod >= 1.6
 *
 * Thread-safe: Yes
 * Reentrant: No
 */

#include "gpio_input.hpp"
#include <chrono>
#include <stdexcept>
#include <pthread.h>
#include <iostream>

/**
 * @brief Default constructor for GPIOInput.
 * @details Initializes internal state and flags to default values:
 *          - gpio_pin_ is set to -1 (unconfigured)
 *          - trigger_high_ is set to false (falling edge by default)
 *          - pull_mode_ is set to PullMode::None
 *          - debounce_triggered_ is cleared
 *          - running_ and stop_thread_ are set to false
 *          - status_ is set to Status::NotConfigured
 *
 * The object remains inactive until enable() is explicitly called.
 */
GPIOInput::GPIOInput() : gpio_pin_(-1),
                             trigger_high_(false),
                             pull_mode_(PullMode::None),
                             debounce_triggered_(false),
                             running_(false),
                             stop_thread_(false),
                             status_(Status::NotConfigured)
{
}

/**
 * @brief Default destructor for GPIOInput.
 * @details calls stop() and terminates threads.
 */
GPIOInput::~GPIOInput()
{
    stop();
}

/**
 * @brief Enable GPIO monitoring.
 *
 * Configures the GPIO pin (using BCM numbering) with the desired trigger condition
 * (true for trigger on rising/high, false for trigger on falling/low), sets the
 * internal pull mode, and registers a callback that is invoked upon detecting an edge.
 *
 * @param pin         BCM GPIO pin number.
 * @param trigger_high True to trigger on a rising edge; false to trigger on a falling edge.
 * @param pull_mode   Desired internal pull mode.
 * @param callback    Callback to be executed when the trigger edge is detected.
 * @return true if the monitor was successfully enabled.
 */
bool GPIOInput::enable(int pin, bool trigger_high, PullMode pull_mode, std::function<void()> callback)
{
    if (running_)
    {
        stop();
    }
    {
        std::lock_guard<std::mutex> lock(monitor_mutex_);
        gpio_pin_ = pin;
        trigger_high_ = trigger_high;
        pull_mode_ = pull_mode;
        callback_ = callback;
        debounce_triggered_ = false;
        stop_thread_ = false;
    }

    try
    {
        // Open the default GPIO chip.
        chip_ = std::make_unique<gpiod::chip>("gpiochip0");
        // Get the line corresponding to the BCM pin.
        line_ = std::make_unique<gpiod::line>(chip_->get_line(gpio_pin_));
        if (!line_)
        {
            throw std::runtime_error("Failed to get GPIO line");
        }

        // Create a line request object to set parameters including bias.
        gpiod::line_request req;
        req.consumer = "GPIOInput";
        // Use an event request type based on the trigger edge desired.
        if (trigger_high_)
        {
            req.request_type = gpiod::line_request::EVENT_RISING_EDGE;
        }
        else
        {
            req.request_type = gpiod::line_request::EVENT_FALLING_EDGE;
        }
        // Set the bias flag if supported.
        req.flags = (pull_mode_ == PullMode::PullUp) ? gpiod::line_request::FLAG_BIAS_PULL_UP : (pull_mode_ == PullMode::PullDown ? gpiod::line_request::FLAG_BIAS_PULL_DOWN : 0);

        // Request the line using the configuration.
        line_->request(req);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in GPIO initialization: " << e.what() << std::endl;
        status_ = Status::Error;
        return false;
    }

    try
    {
        running_ = true;
        status_ = Status::Running;
        // Start the monitoring thread that waits for GPIO events.
        monitor_thread_ = std::thread(&GPIOInput::monitorLoop, this);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error starting monitor thread: " << e.what() << std::endl;
        status_ = Status::Error;
        return false;
    }
    return true;
}

/**
 * @brief Stop the monitoring thread.
 *
 * @return true if the monitor was running and is now stopped.
 */
bool GPIOInput::stop()
{
    if (!running_)
    {
        return false;
    }
    stop_thread_ = true;
    cv_.notify_all();
    if (monitor_thread_.joinable())
    {
        monitor_thread_.join();
    }
    running_ = false;
    status_ = Status::Stopped;
    return true;
}

/**
 * @brief Reset the debounce state.
 *
 * Clears the internal debounce flag so that another trigger may be detected.
 */
void GPIOInput::resetTrigger()
{
    debounce_triggered_ = false;
}

/**
 * @brief Set the CPU priority for the monitoring thread.
 *
 * @param priority Desired CPU priority (e.g., in the SCHED_FIFO range).
 * @return true if the priority was set successfully.
 */
bool GPIOInput::setCPUPriority(int priority)
{
    if (!running_)
    {
        return false;
    }
    pthread_t handle = monitor_thread_.native_handle();
    sched_param sch_params;
    sch_params.sched_priority = priority;
    if (pthread_setschedparam(handle, SCHED_FIFO, &sch_params))
    {
        return false;
    }
    return true;
}

/**
 * @brief Retrieve the current status.
 *
 * @return Status enum indicating the current state.
 */
GPIOInput::Status GPIOInput::getStatus() const
{
    return status_;
}

// Main loop that waits for edge events using libgpiod.
void GPIOInput::monitorLoop()
{
    // This loop uses libgpiod's event_wait() and event_read() to block until an edge event occurs.
    while (!stop_thread_)
    {
        try
        {
            // Wait up to 1 second for an event.
            if (line_->event_wait(std::chrono::seconds(1)))
            {
                // Read the event. Since we requested a specific edge,
                // an event here should represent the desired trigger.
                auto event = line_->event_read();
                if (!debounce_triggered_)
                {
                    debounce_triggered_ = true;
                    if (callback_)
                    {
                        callback_();
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            // On error, update status.
            status_ = Status::Error;
            // Optionally you might break out or continue depending on your design.
        }
    }
}
