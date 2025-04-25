/**
 * @file wspr_scheduler.hpp
 * @brief A C++ class designed to handle the timing of WSPR Transmissions.
 *
 * This file is part of WsprryPi, a project originally branched from
 * @threeme3's WsprryPi project (no longer on GitHub). However, now the
 * original code remains only as a memory and inspiration, and this project
 * is no longer a derivative work.
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

#ifndef WSPR_SCHEDULER_HPP
#define WSPR_SCHEDULER_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

#include <pthread.h>

/**
 * @brief Class for controlling WSPR transmissions.
 *
 * This class manages WSPR transmissions by scheduling and executing transmission routines.
 * It supports two transmission types, allows setting the scheduling policy and thread priority,
 * and provides an externally visible indicator of whether a transmission is active.
 *
 * Transmission routines are scheduled using condition variables and can be interrupted.
 * An optional callback function may be provided during start that will be invoked
 * when the transmission is complete.
 */
class WSPR_Scheduler
{
public:
    /**
     * @brief Enumeration of supported transmission types.
     */
    enum TransmissionType
    {
        WSPR_2,   ///< WSPR transmission type with a 110-second duration.
        WSPR_15   ///< WSPR transmission type with an 825-second duration.
    };

    /**
     * @brief Constructor.
     *
     * Constructs a WSPR_Scheduler object without starting any processing.
     */
    WSPR_Scheduler();

    /**
     * @brief Destructor.
     *
     * Stops any running threads before destruction.
     */
    ~WSPR_Scheduler();

    /**
     * @brief Set the scheduling policy and thread priority for all threads.
     *
     * This setting will be applied to any currently running threads as well as any threads
     * that start after this call.
     *
     * @param policy The scheduling policy (e.g., SCHED_FIFO, SCHED_RR).
     * @param priority The scheduling priority.
     */
    void set_thread_priority(int policy, int priority);

    /**
     * @brief Start transmission with the specified transmission type.
     *
     * This method schedules and starts a transmission routine. An optional callback may be provided
     * to be invoked once the transmission completes.
     *
     * @param type The transmission type (WSPR_2 or WSPR_15).
     * @param transmissionCompleteCallback Optional callback function that is called when the transmission is complete.
     */
    void start(TransmissionType type, std::function<void()> transmissionCompleteCallback = {});

    /**
     * @brief Immediately stop all running threads.
     *
     * This method signals all active threads to stop and waits for them to join.
     */
    void stop();

    /**
     * @brief Notify that the transmission is complete.
     *
     * Called by the transmission threads to mark the transmission as complete,
     * notify waiting threads, and invoke the optional callback.
     */
    void notify_complete();

    /**
     * @brief Check if a transmission is currently active.
     *
     * @return True if a transmission is active, false otherwise.
     */
    bool isTransmitting() const;

    /**
     * @brief Sets transmission active or inactive.
     *
     * @return True if a transmission is active, false otherwise.
     */
    void set_enabled(bool enabled);

private:
    /**
     * @brief Monitor thread function.
     *
     * Waits for the next scheduled transmission time and starts the transmission routine.
     */
    void monitor();

    /**
     * @brief Compute the next scheduled time point for transmission.
     *
     * Computes the next time point at which a transmission should begin based on the selected type.
     *
     * @param type The transmission type (WSPR_2 or WSPR_15).
     * @return The next scheduled time point.
     */
    std::chrono::system_clock::time_point compute_next_schedule(TransmissionType type);

    /**
     * @brief Transmission routine for WSPR_2.
     *
     * Simulates a WSPR_2 transmission that runs for 110 seconds or until interrupted.
     */
    void transmit_wspr2();

    /**
     * @brief Transmission routine for WSPR_15.
     *
     * Simulates a WSPR_15 transmission that runs for 825 seconds or until interrupted.
     */
    void transmit_wspr15();

    /**
     * @brief Apply scheduling parameters to a given thread.
     *
     * Applies the current scheduling policy and thread priority to the specified thread.
     *
     * @param t The thread to modify.
     */
    void apply_thread_priority(std::thread &t);

    // Member variables
    std::atomic<bool> enabled_;              ///< Set to enable or disable transmissions
    int thread_policy_;                      ///< The scheduling policy.
    int thread_priority_;                    ///< The thread priority.
    std::atomic<bool> stop_flag_;            ///< Flag to signal threads to stop.
    std::atomic<bool> transmission_running_; ///< Indicator that a transmission is active.
    TransmissionType transmission_type_;     ///< The selected transmission type.
    std::function<void()> transmissionCompleteCallback_; ///< Optional callback invoked when transmission completes.

    std::thread monitor_thread_;             ///< Thread for monitoring scheduled transmissions.
    std::thread transmission_thread_;        ///< Thread executing the transmission routine.

    std::mutex mtx_;                         ///< Mutex for synchronizing access.
    std::condition_variable cv_;             ///< Condition variable for scheduling and interruption.

    // Disallow copying.
    WSPR_Scheduler(const WSPR_Scheduler &) = delete;
    WSPR_Scheduler &operator=(const WSPR_Scheduler &) = delete;
};

/**
 * @brief Global instance of the WSPR_Scheduler class.
 *
 * This global instance is available project-wide.
 */
extern WSPR_Scheduler wspr_scheduler;

#endif // WSPR_SCHEDULER_HPP
