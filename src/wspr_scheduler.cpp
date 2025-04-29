/**
 * @file wspr_scheduler.hpp
 * @brief A C++ class designed to handle the timing of WSPR Transmissions.
 *
 * This project is is licensed under the MIT License. See LICENSE.MIT.md
 * for more information.
 *
 * Copyright (C) 2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "wspr_scheduler.hpp"

#include "scheduling.hpp"
#include "web_socket.hpp"
#include "wspr_transmit.hpp"
#include "lcblog.hpp"
#include "json.hpp"
#include "version.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>

/**
 * @brief Global WSPR scheduler instance.
 *
 * Provides a single, application-wide scheduler responsible for:
 *   - Monitoring the configuration file for changes
 *   - Computing and triggering WSPR transmission schedules
 *   - Managing transmission and monitor threads
 *
 * This instance should be used by application code to start, stop, and
 * configure scheduled WSPR transmissions.
 */
WSPR_Scheduler wspr_scheduler;

/**
 * @brief Constructs a WSPR_Scheduler with default settings.
 *
 * Initializes the scheduler’s threading policy, priority, and control flags:
 *   - thread_policy_    : default to SCHED_OTHER (non-real-time scheduling)
 *   - thread_priority_  : default to 0 (ignored by SCHED_OTHER)
 *   - stop_flag_        : false (no stop requested)
 *   - transmission_running_ : false (no transmission in progress)
 *   - transmission_type_: WSPR_2 (default 2-minute mode)
 *
 * @note This constructor does not start any threads; call start() to begin scheduling.
 */
WSPR_Scheduler::WSPR_Scheduler()
    : thread_policy_(SCHED_OTHER),
      thread_priority_(0),
      stop_flag_(false),
      transmission_running_(false),
      transmission_type_(WSPR_2)
{
    // No additional work needed at instantiation.
}

/**
 * @brief Destructor for WSPR_Scheduler.
 *
 * Ensures that all background threads (monitor and transmission) are
 * cleanly stopped and joined before the scheduler object is destroyed,
 * preventing potential race conditions or use-after-free errors.
 */
WSPR_Scheduler::~WSPR_Scheduler()
{
    // Stop and join any running threads
    stop();
}

/**
 * @brief Update the scheduling policy and priority for scheduler threads.
 *
 * Stores the new POSIX scheduling policy and priority for any threads
 * launched by this scheduler. If the monitor or transmission threads
 * are already running, their scheduling parameters are updated immediately.
 *
 * @param[in] policy   POSIX thread scheduling policy (e.g., SCHED_FIFO, SCHED_RR, SCHED_OTHER).
 * @param[in] priority Thread priority (1–99 for real-time policies; ignored when using SCHED_OTHER).
 *
 * @note Both monitor and transmission threads will only be updated if they
 *       are currently joinable (i.e., running).
 */
void WSPR_Scheduler::setThreadPriority(int policy, int priority)
{
    std::lock_guard<std::mutex> guard(mtx_);

    thread_policy_ = policy;
    thread_priority_ = priority;

    // Update priority for the monitor thread if it's active
    if (monitor_thread_.joinable())
    {
        apply_thread_priority(monitor_thread_);
    }

    // Update priority for the transmission thread if it's active
    if (transmission_thread_.joinable())
    {
        apply_thread_priority(transmission_thread_);
    }
}

/**
 * @brief Start the WSPR scheduler.
 *
 * Resets control flags, stores the user-provided completion callback,
 * and launches the monitor thread which triggers transmissions at the
 * next scheduled time.
 *
 * @param[in] transmissionCompleteCallback  Callable to invoke once a transmission has finished.
 *
 * @note If a monitor thread is already running or a transmission is in progress,
 *       this call will log and return without starting a new one.
 */
void WSPR_Scheduler::start(std::function<void()> transmissionCompleteCallback)
{
    std::lock_guard<std::mutex> guard(mtx_);

    // Prevent starting if already running
    if (monitor_thread_.joinable() || transmission_running_.load(std::memory_order_acquire))
    {
        llog.logS(INFO, "Transmission already running.");
        return;
    }

    // Reset stop flag for the new run
    stop_flag_.store(false, std::memory_order_release);

    // Store the callback for completion notification
    transmissionCompleteCallback_ = std::move(transmissionCompleteCallback);

    // Launch the monitoring thread to schedule future transmissions
    monitor_thread_ = std::thread(&WSPR_Scheduler::monitor, this);

    // Apply real-time scheduling policy if configured
    apply_thread_priority(monitor_thread_);
}

/**
 * @brief Reset and apply the initial transmission configuration.
 *
 * Resets the round-robin frequency iterator, fetches the first frequency
 * from `config.center_freq_set`, and invokes the transmitter setup with
 * that frequency.
 *
 * This should be called before starting the scheduler’s monitor loop,
 * or whenever the frequency list has changed.
 */
void WSPR_Scheduler::setConfig()
{
    // Start iteration from the beginning of the frequency list.
    freq_iterator_ = 0;

    // 2) Retrieve the first (or only) frequency.
    current_frequency_ = next_frequency();

    // Apply the configuration to the transmitter.
    //    `set_config(double)` is expected to configure `wsprTransmitter`
    //    (e.g., via setupTransmission) and any related parameters.
    set_config(current_frequency_);
}

/**
 * @brief Apply updated transmission parameters and reinitialize DMA.
 *
 * Retrieves the current PPM value if NTP calibration is enabled, captures
 * the latest configuration settings, and reconfigures the WSPR transmitter
 * with the specified frequency and parameters.
 *
 * @param freq_hz Center frequency for the upcoming transmission, in Hertz.
 *
 * @throws std::runtime_error if DMA setup or mailbox operations fail within
 *         `setupTransmission()`.
 */
void WSPR_Scheduler::set_config(double freq_hz)
{
    llog.logS(DEBUG, "Retrieving PPM.");
    if (config.use_ntp) {
        config.ppm = ppmManager.getCurrentPPM();
    }

    // Copy current settings locally to avoid data races
    double frequency    = freq_hz;
    int    powerLevel   = config.power_level;
    double ppmValue     = config.ppm;
    std::string callSign = config.callsign;
    std::string gridLoc  = config.grid_square;
    int    powerDbm     = config.power_dbm;
    bool   useOffset    = config.use_offset;

    llog.logS(DEBUG, "Setting DMA.");
    wsprTransmitter.setupTransmission(
        frequency,
        powerLevel,
        ppmValue,
        std::move(callSign),
        std::move(gridLoc),
        powerDbm,
        useOffset
    );
}
/**
 * @brief Stop the scheduler and clean up threads.
 *
 * This method:
 *   1. Acquires the lock on `mtx_`.
 *   2. Sets the stop flag to true to signal any waiting operations.
 *   3. Notifies all threads waiting on `cv_` so they can wake and exit.
 *   4. Releases the lock.
 *   5. Joins the monitor thread and the transmission thread if they are running.
 *
 * @note It is safe to call this multiple times; subsequent calls will have no effect
 *       once threads have been joined.
 */
void WSPR_Scheduler::stop()
{
    {
        std::lock_guard<std::mutex> guard(mtx_);
        // Signal stop to any waiting loops
        stop_flag_.store(true, std::memory_order_release);
        cv_.notify_all();
    }

    // Join the file-monitoring thread if it's running
    if (monitor_thread_.joinable())
    {
        monitor_thread_.join();
    }

    // Join the transmission thread if it's running
    if (transmission_thread_.joinable())
    {
        transmission_thread_.join();
    }
}

/**
 * @brief Immediately stops transmissions.
 *
 * This method will cause all active transmissions to end immediately.
 */
void WSPR_Scheduler::stopTransmission()
{
    wsprTransmitter.stopTransmission();
}

/**
 * @brief Notify that the transmission has completed.
 *
 * This method:
 *   1. Acquires the lock on `mtx_`.
 *   2. Marks `transmission_running_` as false.
 *   3. Wakes any threads waiting on `cv_`.
 *   4. Copies (and clears) the user-supplied completion callback.
 *   5. Releases the lock.
 *   6. Invokes the callback, if one was provided, outside of the lock to
 *      avoid potential deadlocks.
 */
void WSPR_Scheduler::notify_complete()
{
    std::function<void()> callback;

    {
        std::lock_guard<std::mutex> guard(mtx_);
        // Mark transmission as no longer running
        transmission_running_.store(false, std::memory_order_release);
        // Wake any waiting threads
        cv_.notify_all();
        // Capture the callback for invocation outside the lock
        callback = std::move(transmissionCompleteCallback_);
    }

    // Invoke callback (if set) without holding the lock
    if (callback)
    {
        callback();
    }
}

/**
 * @brief Retrieve the next center frequency, cycling through the configured list.
 *
 * This method returns the next frequency from `config.center_freq_set` in a
 * round-robin fashion.  It uses `freq_iterator_` modulo the list size to
 * index into the vector, then increments `freq_iterator_` for the subsequent call.
 *
 * @return double
 *   - Next frequency in Hz from the list.
 *   - Returns 0.0 if the list is empty.
 *
 * @note
 *   - `freq_iterator_` should be initialized to 0.
 *   - Wrapping is handled via the modulo operation.
 */
double WSPR_Scheduler::next_frequency()
{
    const auto &freqs = config.center_freq_set;
    if (freqs.empty())
    {
        return 0.0;
    }

    // Compute index in [0, freqs.size())
    const size_t idx = freq_iterator_ % freqs.size();

    // Fetch and advance iterator
    double freq = freqs[idx];
    ++freq_iterator_;

    return freq;
}

/**
 * @brief Main loop that schedules and triggers WSPR transmissions.
 *
 * This method continuously computes the next transmission time based on
 * the configured `transmission_type_`, waits until that time or until
 * `stop()` is called, and then:
 *  - Advances to the next frequency in round-robin fashion.
 *  - Reconfigures the DMA/transmitter if the frequency changed.
 *  - Launches the appropriate transmission thread (WSPR2 or WSPR15).
 *  - Waits for the transmission to complete before scheduling the next one.
 *
 * The loop exits when `stop_flag_` is set.
 */
void WSPR_Scheduler::monitor()
{
    while (true)
    {
        // Determine when the next transmission should occur.
        auto next_schedule = compute_next_schedule(transmission_type_);

        {
            // Wait without busy-waiting until the scheduled time or stop() is invoked.
            std::unique_lock<std::mutex> lock(mtx_);
            if (cv_.wait_until(lock, next_schedule, [this] { return stop_flag_.load(); })) {
                break;  // stop_flag_ was set during the wait
            }
        }

        // If stop() was called after the wait, exit the loop.
        if (stop_flag_.load()) {
            break;
        }

        // Retrieve the next frequency; if it differs, reconfigure the transmitter.
        double freq = next_frequency();
        if (freq != current_frequency_) {
            current_frequency_ = freq;
            if (current_frequency_ > 0.0) {
                set_config(current_frequency_);
            }
        }

        // Only start a transmission if enabled and a valid frequency is set.
        if (enabled_.load() && current_frequency_ > 0.0) {
            // Launch the correct transmission routine in a dedicated thread.
            if (transmission_type_ == WSPR_2) {
                transmission_thread_ = std::thread(&WSPR_Scheduler::transmit_wspr2, this);
            } else {
                transmission_thread_ = std::thread(&WSPR_Scheduler::transmit_wspr15, this);
            }

            // Apply real-time priority settings to the new thread.
            apply_thread_priority(transmission_thread_);

            // Block until the transmission finishes.
            if (transmission_thread_.joinable()) {
                transmission_thread_.join();
            }
        } else {
            // No valid frequency or disabled: skip this cycle.
            llog.logS(INFO, "Skipping transmission based on 0 in sequence.");
        }
    }
}

/**
 * @brief Compute the next WSPR transmission start time, aligned to protocol slots.
 *
 * Given the current local time, this function returns the next valid start
 * time for either a 2-minute or 15-minute WSPR transmission.  The scheduled
 * second is always set to 1, per WSPR timing conventions.
 *
 * @param[in] type  The transmission type: WSPR_2 (every even minute) or
 *                  WSPR_15 (every quarter‐hour).
 * @return std::chrono::system_clock::time_point
 *         The next scheduled start time on or after the next valid slot.
 *
 * @note Uses local system time.  Assumes TransmissionType is an enum with
 *       values WSPR_2 and WSPR_15.
 */
std::chrono::system_clock::time_point
WSPR_Scheduler::compute_next_schedule(TransmissionType type)
{
    using namespace std::chrono;

    // Capture current time
    auto now = system_clock::now();
    std::time_t tt = system_clock::to_time_t(now);
    std::tm localTM = *std::localtime(&tt);

    // Always schedule at second = 1
    localTM.tm_sec = 1;

    // Align minute according to transmission type
    switch (type)
    {
    case WSPR_2:
    {
        // Next even minute boundary
        if (localTM.tm_min % 2 != 0)
        {
            ++localTM.tm_min;
        }
        break;
    }

    case WSPR_15:
    {
        // Next 15-minute boundary (multiples of 15)
        int rem = localTM.tm_min % 15;
        if (rem != 0)
        {
            localTM.tm_min += (15 - rem);
        }
        break;
    }

    default:
        // Unknown type: fall back to next minute
        ++localTM.tm_min;
        break;
    }

    // Convert back to time_point and ensure it's in the future
    auto scheduled = system_clock::from_time_t(std::mktime(&localTM));
    if (scheduled <= now)
    {
        // If we've just passed that slot, advance by the period
        if (type == WSPR_2)
        {
            scheduled += minutes(2);
        }
        else if (type == WSPR_15)
        {
            scheduled += minutes(15);
        }
    }

    return scheduled;
}

/**
 * @brief Runs a 2-minute WSPR-2 transmission session.
 *
 * This method:
 *   1. Notifies observers that transmission is starting (via WebSocket and log).
 *   2. Marks the transmission as running.
 *   3. Kicks off the low-level WSPR-2 transmit loop.
 *   4. Waits up to 110 seconds—or until a stop flag is set—to allow the transmission to complete.
 *   5. Cleans up state, logs completion, and notifies observers.
 *
 * @note The blocking call to `wsprTransmitter.transmit()` returns once the WSPR symbol
 *       sequence has been sent; the subsequent wait enforces the full 110 s duration (or
 *       earlier if `stop_flag_` is set), ensuring consistent high-level timing.
 */
void WSPR_Scheduler::transmit_wspr2()
{
    // Notify clients and log start
    send_ws_message("transmit", "starting");
    llog.logS(INFO, "Starting a WSPR-2 transmission.");

    // Mark the transmission as active
    transmission_running_.store(true);

    // Record start time and maximum duration (110 seconds ≈ 2 min – buffer)
    const auto start_time = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds{110};

    // Begin the low-level transmit loop (returns after symbols sent)
    wsprTransmitter.transmit();

    // Wait until either the timeout elapses or stop_flag_ is raised
    {
        std::unique_lock<std::mutex> lock{mtx_};
        cv_.wait_until(
            lock,
            start_time + timeout,
            [this]() noexcept
            { return stop_flag_.load(); });
    }

    // Mark the transmission as finished
    transmission_running_.store(false);
    llog.logS(INFO, "WSPR-2 transmission ending.");

    // Invoke any completion callbacks
    notify_complete();

    // Notify clients of end
    send_ws_message("transmit", "finished");
}

/**
 * @brief Runs a 15-minute WSPR transmission session.
 *
 * This method:
 *   1. Notifies any observers that transmission is starting (via WebSocket and log).
 *   2. Marks the transmission as running.
 *   3. Kicks off the low-level WSPR-15 transmit loop.
 *   4. Waits up to 825 seconds—or until a stop flag is set—to allow the transmission to complete.
 *   5. Cleans up state, logs completion, and notifies observers.
 *
 * @note The blocking call to `wsprTransmitter.transmit()` will return once the WSPR symbol
 *       sequence has been sent; the subsequent wait enforces the full 825 s duration (or earlier
 *       if `stop_flag_` is set), so that higher-level timing remains consistent.
 */
void WSPR_Scheduler::transmit_wspr15()
{
    // Notify clients and log start
    send_ws_message("transmit", "starting");
    llog.logS(INFO, "Starting a WSPR-15 transmission.");

    // Mark the transmission as active
    transmission_running_.store(true);

    // Remember when we began
    const auto start_time = std::chrono::steady_clock::now();
    // 825 seconds ≈ 15 minutes + 15 s guard
    const auto timeout = std::chrono::seconds{825};

    // Begin the low-level transmit loop (will return after symbols sent)
    wsprTransmitter.transmit();

    // Now wait until either the full period elapses or stop_flag_ is raised
    {
        std::unique_lock<std::mutex> lock{mtx_};
        cv_.wait_until(
            lock,
            start_time + timeout,
            [this]() noexcept
            { return stop_flag_.load(); });
    }

    // Mark the transmission as finished
    transmission_running_.store(false);
    llog.logS(INFO, "WSPR-15 transmission ending.");

    // Fire any completion callbacks
    notify_complete();

    // Notify clients of end
    send_ws_message("transmit", "finished");
}

/**
 * @brief Applies the configured POSIX scheduling policy and priority to a thread.
 *
 * This function sets the thread’s scheduling parameters (policy and priority)
 * to match the scheduler’s configuration. If the thread is not joinable,
 * no action is taken.
 *
 * @param[in,out] t  The `std::thread` whose native handle will be adjusted.
 *
 * @note Requires that the program be run with appropriate privileges
 *       (e.g., CAP_SYS_NICE) to set real-time priorities on Linux.
 */
void WSPR_Scheduler::apply_thread_priority(std::thread &t)
{
    if (!t.joinable())
    {
        return;
    }

    // Obtain the native pthread handle
    pthread_t nativeHandle = t.native_handle();

    // Prepare scheduling parameters
    sched_param schParams{};
    schParams.sched_priority = thread_priority_;

    // Attempt to set the thread’s scheduling policy and priority
    int ret = pthread_setschedparam(nativeHandle, thread_policy_, &schParams);
    if (ret != 0)
    {
        llog.logS(WARN,
                  "Failed to set thread scheduling: policy=",
                  thread_policy_,
                  ", priority=",
                  thread_priority_,
                  " (errno=",
                  ret,
                  ")");
    }
}

/**
 * @brief Provides informaiton that transmission is active
 *
 * @return True if a transmission is active, false otherwise.
 */
bool WSPR_Scheduler::is_transmitting() const
{
    return wsprTransmitter.isTransmitting();
}

/**
 * @brief Sets transmission active or inactive.
 *
 * @return True if a transmission is active, false otherwise.
 */
void WSPR_Scheduler::setEnabled(bool enabled)
{
    enabled_.store(enabled);
}

/**
 * @brief Broadcasts a JSON-formatted WebSocket message to all connected clients.
 *
 * Builds a JSON object containing a message type, state, and current UTC
 * timestamp (ISO 8601), serializes it, and sends it over the WebSocket server.
 *
 * @param[in] type   The message category (e.g., "transmit", "status").
 * @param[in] state  The message state or payload (e.g., "starting", "finished").
 *
 * @note Requires <nlohmann/json.hpp>, <chrono>, <ctime>, <iomanip>, and <sstream>.
 */
void WSPR_Scheduler::send_ws_message(std::string type, std::string state)
{
    // Build JSON payload
    nlohmann::json j;
    j["type"] = type;
    j["state"] = state;

    // Capture current UTC time and format as ISO 8601 (YYYY-MM-DDThh:mm:ssZ)
    auto now = std::chrono::system_clock::now();
    auto now_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_utc{};
    gmtime_r(&now_t, &tm_utc);

    std::ostringstream oss;
    oss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");
    j["timestamp"] = oss.str();

    // Serialize and send to all WebSocket clients
    const std::string message = j.dump();
    socketServer.sendAllClients(message);
}
