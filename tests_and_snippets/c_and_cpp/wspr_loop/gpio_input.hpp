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

#ifndef SHUTDOWN_HANDLER_HPP
#define SHUTDOWN_HANDLER_HPP

#include <atomic>
#include <condition_variable>
#include <functional>
#include <gpiod.hpp>
#include <memory>
#include <mutex>
#include <thread>

class GPIOInput
{
public:
    // Enumeration for configuring the line's bias.
    enum class PullMode
    {
        None,
        PullUp,
        PullDown
    };
    // Status to indicate the current operational state.
    enum class Status
    {
        NotConfigured,
        Running,
        Stopped,
        Error
    };

    // Default constructor and destructor.
    GPIOInput();
    ~GPIOInput();

    // Delete copy operations to avoid hardware conflicts.
    GPIOInput(const GPIOInput &) = delete;
    GPIOInput &operator=(const GPIOInput &) = delete;

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
    bool enable(int pin, bool trigger_high, PullMode pull_mode, std::function<void()> callback);

    /**
     * @brief Stop the monitoring thread.
     *
     * @return true if the monitor was running and is now stopped.
     */
    bool stop();

    /**
     * @brief Reset the debounce state.
     *
     * Clears the internal debounce flag so that another trigger may be detected.
     */
    void resetTrigger();

    /**
     * @brief Sets the scheduling policy and priority of the signal handling thread.
     *
     * @details
     * Uses `pthread_setschedparam()` to adjust the real-time scheduling policy and
     * priority of the signal handling worker thread.
     *
     * This function is useful for raising the importance of the signal handling
     * thread under high system load, especially when using `SCHED_FIFO` or
     * `SCHED_RR`.
     *
     * @param schedPolicy The scheduling policy (e.g., `SCHED_FIFO`, `SCHED_RR`, `SCHED_OTHER`).
     * @param priority The thread priority value to assign (depends on policy).
     *
     * @return `true` if the scheduling parameters were successfully applied,
     *         `false` otherwise (e.g., thread not running or `pthread_setschedparam()` failed).
     *
     * @note
     * The caller may require elevated privileges (e.g., CAP_SYS_NICE) to apply real-time priorities.
     * It is the caller's responsibility to ensure the priority value is valid for the given policy.
     */
    bool setPriority(int schedPolicy, int priority);

    /**
     * @brief Retrieve the current status.
     *
     * @return Status enum indicating the current state.
     */
    Status getStatus() const;

private:
    // Main loop that waits for edge events using libgpiod.
    void monitorLoop();

    // Configuration parameters.
    int gpio_pin_;
    bool trigger_high_;
    PullMode pull_mode_;
    std::function<void()> callback_;

    // Internal state flags.
    std::atomic<bool> debounce_triggered_;
    std::atomic<bool> running_;
    std::atomic<bool> stop_thread_;

    // Thread handling.
    std::thread monitor_thread_;
    std::mutex monitor_mutex_;
    std::condition_variable cv_;

    // Current status.
    Status status_;

    // libgpiod objects to access the GPIO chip and line.
    std::unique_ptr<gpiod::chip> chip_;
    std::unique_ptr<gpiod::line> line_;
};

// Global instance
extern GPIOInput shutdownMonitor;

#endif // SHUTDOWN_HANDLER_HPP