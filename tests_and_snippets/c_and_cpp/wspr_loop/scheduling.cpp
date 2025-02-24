/**
 * @file scheduling.cpp
 * @brief Manages transmit, INI monitoring and scheduling for Wsprry Pi
 *
 * This file is part of WsprryPi, a project originally created from @threeme3
 * WsprryPi projet (no longer on GitHub). However, now the original code
 * remains only as a memory and inspiration, and this project is no longer
 * a deriivative work.
 *
 * This project is is licensed under the MIT License. See LICENSE.MIT.md
 * for more information.
 *
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
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

 // Primary header for this source file
#include "scheduling.hpp"

// Project headers
#include "arg_parser.hpp"
#include "constants.hpp"
#include "signal_handler.hpp"
#include "transmit.hpp"

// Standard library headers
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// System headers
#include <string.h>
#include <sys/resource.h>

/**
 * @brief Condition variable used for thread synchronization.
 *
 * This condition variable allows the scheduler thread to wait for
 * specific conditions, such as the exit signal or the next transmission
 * interval.
 */
std::condition_variable cv;

/**
 * @brief Mutex for protecting access to shared resources.
 *
 * This mutex is used to synchronize access to shared data between threads,
 * ensuring thread safety when accessing the condition variable.
 */
std::mutex cv_mtx;

/**
 * @brief Atomic flag indicating when the scheduler should exit.
 *
 * This flag allows threads to gracefully exit by checking its value.
 * It is set to `true` when the program needs to terminate the scheduler
 * and associated threads.
 */
std::atomic<bool> exit_scheduler(false);

/**
 * @brief Thread handle for the scheduler.
 *
 * This thread runs the `scheduler_thread` function, managing the timing
 * of WSPR transmissions and ensuring they occur at the correct intervals.
 */
std::thread schedulerThread;

/**
 * @brief Stores the names of all detected CPU cores.
 *
 * This static vector holds the names of the available CPU cores on the system.
 * It is populated by `discover_cpu_cores()` and used for frequency checks in
 * the `is_cpu_throttled()` function.
 */
static std::vector<std::string> all_cpu_cores;

/**
 * @brief Checks if the directory name represents a real CPU directory (e.g. "cpu0").
 *
 * We do a quick check:
 * - Must start with "cpu"
 * - Must have at least 4 characters ("cpu0" is the shortest)
 * - All characters after "cpu" must be digits.
 */
bool is_cpu_directory(const std::string &dirName)
{
    // Must start with "cpu"
    if (dirName.rfind("cpu", 0) != 0) // or dirName.compare(0, 3, "cpu") == 0
        return false;

    // Must have at least one digit after "cpu"
    if (dirName.size() < 4) // e.g. "cpu0" is length 4
        return false;

    // Check if characters after "cpu" are digits
    for (size_t i = 3; i < dirName.size(); ++i)
    {
        if (!std::isdigit(static_cast<unsigned char>(dirName[i])))
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief Populates the list of valid CPU core directories.
 *
 * This function scans `/sys/devices/system/cpu/` and stores directory names like
 * "cpu0", "cpu1", etc. in the global `all_cpu_cores` vector for later use.
 */
void discover_cpu_cores()
{
    // Avoid re-discovery if already populated
    if (!all_cpu_cores.empty()) return;

    for (const auto &entry : std::filesystem::directory_iterator("/sys/devices/system/cpu/"))
    {
        // We only care about directories
        if (!entry.is_directory())
            continue;

        std::string dirName = entry.path().filename().string();

        // If it's a real CPU directory like "cpu0", "cpu1", etc.
        if (is_cpu_directory(dirName))
        {
            all_cpu_cores.push_back(dirName);
        }
    }
}

/**
 * @brief Checks if any CPU core is throttled due to temperature or frequency.
 *
 * This function evaluates CPU throttling by:
 * - Checking the highest CPU temperature from `/sys/class/thermal/thermal_zone0/temp`.
 * - Verifying the current frequency of each CPU core from
 *   `/sys/devices/system/cpu/n/cpufreq/scaling_cur_freq`.
 *
 * Logs warnings if the temperature exceeds 80°C or if any core runs below its
 * default frequency based on the detected processor type.
 *
 * @note Ensure the application has sufficient permissions to read the required
 *       system files. Missing permissions or invalid paths will log errors.
 *
 * @return true if any CPU throttling is detected (temperature or frequency).
 * @return false if no throttling is detected and all cores operate normally.
 */
bool is_cpu_throttled()
{
    // Ensure core discovery if not already populated.
    if (all_cpu_cores.empty())
    {
        discover_cpu_cores();
    }

    // Can get the default CPU frequency based on processor type.
    // 600000000 is probably good enough as a general test
    int defaultFrequencyHz = 600000000;
    if (defaultFrequencyHz == 0)
    {
        llog.logE(ERROR, "Failed to determine default CPU frequency.");
        return false;  // Cannot determine throttling without a valid threshold.
    }

    // Read the current CPU temperature.
    std::ifstream temp_file("/sys/class/thermal/thermal_zone0/temp");
    int highest_temp = 0;

    if (temp_file.is_open())
    {
        int current_temp = 0;
        temp_file >> current_temp;
        temp_file.close();
        highest_temp = current_temp;
    }
    else
    {
        llog.logE(ERROR, "Failed to read CPU temperature.");
        return false;  // Assume no throttling if temperature cannot be checked.
    }

    // Track throttling status.
    bool throttled = false;

    // Check each CPU core's current frequency.
    for (const auto& core : all_cpu_cores)
    {
        std::string freq_path = "/sys/devices/system/cpu/" + core + "/cpufreq/scaling_cur_freq";
        std::ifstream freq_file(freq_path);
        int cur_freq = 0;

        if (freq_file.is_open())
        {
            freq_file >> cur_freq;
            freq_file.close();

            // Detect frequency throttling if below the default frequency.
            if (cur_freq < defaultFrequencyHz)
            {
                throttled = true;
            }
        }
        else
        {
            llog.logE(ERROR, "Failed to read current frequency for core", core);
        }
    }

    // Check for temperature throttling (>80°C).
    constexpr int kMaxSafeTempMilliC = 80000;  // 80°C threshold.
    if (highest_temp > kMaxSafeTempMilliC)
    {
        llog.logE(WARN, "CPU temperature throttling detected. Highest temp:",
                  highest_temp / 1000, "°C.");
        throttled = true;
    }

    // Log normal operation if no throttling is detected.
    if (!throttled)
    {
        llog.logS(DEBUG, "All CPU cores running normally. Highest temp:",
                  highest_temp / 1000, "°C. Frequency threshold:",
                  defaultFrequencyHz / 1000000, "MHz.");
    }

    return throttled;
}

/**
 * @brief Sets the current transmission thread to real-time priority.
 *
 * This function configures the calling thread to use the `SCHED_FIFO`
 * scheduling policy with a mid-to-high priority level of 75. This ensures
 * that the transmission thread receives preferential CPU time, reducing
 * latency and jitter during critical operations.
 *
 * If setting the real-time priority fails, an error is logged with the
 * corresponding system error message.
 *
 * @note This function requires appropriate system privileges (e.g., CAP_SYS_NICE)
 *       to modify thread priorities. Without elevated permissions, the call
 *       to `pthread_setschedparam()` will fail.
 *
 */
void set_transmission_realtime()
{
    // Define scheduling parameters.
    struct sched_param sp;
    sp.sched_priority = 75;  // Mid-high priority (range: 1–99).

    // Attempt to set real-time priority for the current thread.
    int ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
    if (ret != 0)
    {
        // Log an error if the priority change fails.
        llog.logE(ERROR, "Failed to set real-time priority for transmission: ", strerror(ret));
    }
    else
    {
        // Confirm success if the priority change succeeds.
        llog.logS(DEBUG, "Transmission thread set to real-time using SCHED_FIFO.");
    }
}

/**
 * @brief Waits until the next transmission interval boundary and triggers transmission.
 *
 * This function waits until one second after the next interval boundary, ensuring
 * that the transmission aligns with the desired schedule. It supports WSPR-2 and
 * WSPR-15 modes, defined by the interval parameter.
 *
 * @param interval The transmission interval in minutes (e.g., 2 for WSPR-2, 15 for WSPR-15).
 * @return true if the wake-up occurred at the expected interval.
 * @return false if interrupted by the exit scheduler or if the target interval was missed.
 *
 * @note This function relies on the `exit_scheduler` atomic flag to exit early.
 *       Ensure `exit_scheduler` is defined and managed appropriately.
 */
bool wait_every(int interval)
{
    using namespace std::chrono;

    // Get the current system time.
    auto now = system_clock::now();

    // Convert current time to integral minutes since epoch.
    auto current_min_time = time_point_cast<minutes>(now);
    auto total_minutes = duration_cast<minutes>(current_min_time.time_since_epoch()).count();

    // Calculate the next interval boundary.
    auto next_boundary = ((total_minutes / interval) + 1) * interval;
    auto next_boundary_time = system_clock::time_point{minutes(next_boundary)};
    auto desired_time = next_boundary_time + seconds(1);  // Start transmission 1 second after boundary.

    // Ensure the desired time is in the future.
    if (desired_time <= now)
    {
        desired_time += minutes(interval);
    }

    // Convert desired time to human-readable format.
    auto desired_time_t = system_clock::to_time_t(desired_time);
    auto desired_tm = *std::gmtime(&desired_time_t);

    // Determine mode string based on interval.
    std::string mode_string = (interval == WSPR_2) ? "WSPR-2"
                           : (interval == WSPR_15) ? "WSPR-15"
                                                   : "Unknown mode";

    // Log the next transmission time.
    std::ostringstream info_message;
    info_message << "Next transmission at "
                 << std::setw(2) << std::setfill('0') << desired_tm.tm_hour << ":"
                 << std::setw(2) << std::setfill('0') << desired_tm.tm_min << ":"
                 << "01 UTC (" << mode_string << ").";
    llog.logS(INFO, info_message.str());

    // Sleep until the desired time while monitoring exit conditions.
    while (system_clock::now() < desired_time)
    {
        if (exit_scheduler.load())
        {
            llog.logE(WARN, "Transmission wait interrupted by exit scheduler.");
            return false;
        }
        std::this_thread::sleep_for(milliseconds(100));
    }

    // Measure the actual wake-up time.
    auto final_time = system_clock::now();
    auto final_t = system_clock::to_time_t(final_time);
    auto final_tm = *std::gmtime(&final_t);

    // Calculate the offset from the desired wake-up time.
    auto offset_ns = duration_cast<nanoseconds>(final_time - desired_time).count();
    double offset_ms = offset_ns / 1e6;

    std::ostringstream offset_message;
    offset_message << "Offset from desired target: "
                   << (offset_ms >= 0 ? "+" : "-")
                   << std::fixed << std::setprecision(4)
                   << std::abs(offset_ms) << " ms.";

    // Verify if the wake-up happened within the expected interval.
    if ((final_tm.tm_min % interval == 0) && final_tm.tm_sec >= 1)
    {
        llog.logS(DEBUG, "Transmission triggered at the target interval (" + std::to_string(interval) + " min).");
        llog.logS(DEBUG, offset_message.str());
        return true;
    }

    // Log an error if the target interval was missed.
    llog.logE(ERROR, "Missed target interval. " + offset_message.str());
    return false;
}

/**
 * @brief Sets the scheduling priority for the current process.
 *
 * This function increases the process priority by setting its nice value to -10.
 * A lower nice value increases priority, ensuring the housekeeping thread receives
 * more CPU time when competing with other processes.
 *
 * If the priority change fails, an error message is logged with the corresponding
 * system error description.
 *
 * @note Changing priority requires appropriate system privileges. Without sufficient
 *       permissions (CAP_SYS_NICE), the call to `setpriority()` will fail.
 *
 * @return void
 */
void set_scheduling_priority()
{
    // Attempt to set the process priority to -10 (higher priority).
    if (setpriority(PRIO_PROCESS, 0, -10) == -1)
    {
        // Log an error if the priority change fails.
        llog.logE(ERROR, "Failed to set scheduler priority: ", std::string(strerror(errno)));
    }
    else
    {
        // Log success if the priority change succeeds.
        llog.logS(INFO, "Housekeeping priority set to -10 (increased priority).");
    }
}

/**
 * @brief Manages the scheduling of WSPR transmissions.
 *
 * This function runs as a dedicated thread, setting an elevated priority (-10)
 * to ensure timely execution. It waits for the next transmission interval based
 * on the configured `wspr_interval` and triggers the transmission process.
 *
 * During each cycle, it:
 * - Retrieves the current WSPR interval.
 * - Waits until the next interval boundary using `wait_every()`.
 * - Initiates a transmission using `transmit()`.
 * - Checks for CPU throttling using `is_cpu_throttled()`.
 *
 * The thread exits when the `exit_scheduler` atomic flag is set.
 *
 * @note Requires appropriate system privileges for priority adjustments.
 *
 * @return void
 */
void scheduler_thread()
{
    // Set scheduler priority to -10 (higher priority).
    if (setpriority(PRIO_PROCESS, 0, -10) == -1)
    {
        llog.logE(ERROR, "Failed to set scheduler thread priority: ", std::string(strerror(errno)));
    }
    else
    {
        llog.logS(DEBUG, "Scheduler thread priority set.");
    }

    llog.logS(DEBUG, "Scheduler thread started.");

    // Lock for condition variable synchronization.
    std::unique_lock<std::mutex> lock(cv_mtx);

    // Main scheduling loop.
    while (!exit_scheduler)
    {
        // Retrieve the current WSPR interval.
        int current_interval = wspr_interval.load();
        llog.logS(DEBUG, "Current WSPR interval set to: ", current_interval, " minutes.");

        // Wait until the next transmission interval.
        if (wait_every(current_interval))
        {
            transmit();         // Start transmission.
            is_cpu_throttled(); // Check for CPU throttling.
        }

        // Wait for exit signal or timeout.
        cv.wait_for(lock, std::chrono::seconds(1), [] { return exit_scheduler.load(); });
    }

    llog.logS(DEBUG, "Scheduler thread exiting.");
}

/**
 * @brief Manages the main WSPR transmission loop.
 *
 * This function runs the primary WSPR transmission loop, coordinating the
 * INI monitor thread and the scheduler thread. It continuously:
 * - Starts the `ini_monitor_thread` to detect INI file changes.
 * - Spawns the `scheduler_thread` to manage transmission intervals.
 * - Waits for the scheduler to signal transmission readiness.
 * - Initiates a transmission when the interval is reached.
 *
 * The loop exits when the `exit_scheduler` atomic flag is set.
 *
 * @note This function runs until externally signaled to stop.
 *
 * @return void
 */
void wspr_loop()
{
    // Start the INI monitor thread.
    iniMonitorThread = std::thread(ini_monitor_thread);

    // Main loop to manage scheduler and transmissions.
    while (!exit_scheduler.load())
    {
        exit_scheduler = false;  // Reset exit flag for the new cycle.

        // Start the scheduler thread.
        schedulerThread = std::thread(scheduler_thread);

        // Wait for the scheduler to signal transmission readiness.
        {
            std::unique_lock<std::mutex> lock(cv_mtx);
            cv.wait(lock, [] { return exit_scheduler.load(); });
        }

        // Join the scheduler thread once signaled.
        if (schedulerThread.joinable())
        {
            schedulerThread.join();
        }

        // Trigger transmission if not exiting.
        if (!exit_scheduler.load())
        {
            transmit();
        }
    }

    // Signal the INI monitor thread to terminate.
    exit_scheduler.store(true);

    // Join the INI monitor thread to ensure a clean exit.
    if (iniMonitorThread.joinable())
    {
        iniMonitorThread.join();
    }
}
