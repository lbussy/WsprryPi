#include "scheduling.hpp"

#include "arg_parser.hpp"
#include "transmit.hpp"
#include "constants.hpp"
#include "signal_handler.hpp"

#include <sys/resource.h>
#include <string.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <atomic>
#include <thread>
#include <condition_variable>

std::condition_variable cv;
std::mutex cv_mtx;
std::atomic<bool> exit_scheduler(false);
std::vector<std::thread> active_threads;

void set_scheduling_priority()
{
    if (setpriority(PRIO_PROCESS, 0, -10) == -1)
    {
        llog.logE(ERROR, "Failed to set scheduler priority:", std::string(strerror(errno)));
    }
    else
    {
        llog.logS(INFO, "Housekeeping priority set to -10 (increased priority).");
    }
}

void set_transmission_realtime()
{
    struct sched_param sp;
    sp.sched_priority = 75; // Choose a priority in the mid-high range (1–99)

    int ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
    if (ret != 0)
    {
        llog.logE(ERROR, "Failed to set real-time priority for transmission:", strerror(ret));
    }
    else
    {
        llog.logS(DEBUG, "Transmission thread set to real-time for FIFO.");
    }
}

std::string get_current_governor()
{
    std::ifstream file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    std::string governor;
    if (file.is_open())
    {
        file >> governor;
        file.close();
    }
    return governor;
}

bool set_cpu_governor(const std::string &governor)
{
    std::ofstream file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    if (file.is_open())
    {
        file << governor;
        file.close();
        llog.logS(INFO, "CPU governor set to:", governor);
        return true;
    }
    llog.logE(ERROR, "Failed to set CPU governor to:", governor);
    return false;
}

void enable_performance_mode()
{
    config.previous_governor = get_current_governor();
    if (config.previous_governor.empty())
    {
        llog.logE(ERROR, "Failed to retrieve current CPU governor.");
        return;
    }

    if (config.previous_governor != "performance")
    {
        if (set_cpu_governor("performance"))
        {
            llog.logS(INFO, "Performance mode enabled.");
        }
    }
    else
    {
        // TODO: Restore the mode in the config file, this got abandoned during testing
        llog.logS(DEBUG, "CPU already in performance mode.");
    }
}

void restore_governor()
{
    if (!config.previous_governor.empty() && config.previous_governor != "performance")
    {
        if (set_cpu_governor(config.previous_governor))
        {
            llog.logS(INFO, "CPU governor restored to:", config.previous_governor);
        }
    }
}

void precise_sleep_until(const timespec &target)
{
    while (true)
    {
        int res = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &target, nullptr);
        if (res == 0)
        {
            break; // Success
        }
        if (res != EINTR)
        {
            llog.logE(ERROR, "clock_nanosleep failed:", std::string(strerror(res)));
            break;
        }
        // If interrupted, continue sleeping until the target time
    }
}

bool wait_every(int interval)
{
    using namespace std::chrono;

    auto now = system_clock::now();

    // Convert current time to integral minutes since epoch
    auto current_min_time = time_point_cast<minutes>(now);
    auto total_minutes = duration_cast<minutes>(current_min_time.time_since_epoch()).count();

    // Calculate next boundary minute
    auto next_boundary = ((total_minutes / interval) + 1) * interval;

    // Next boundary time in <system_clock, minutes>
    auto next_boundary_time = system_clock::time_point{minutes(next_boundary)};

    // Add one second for the desired target time
    auto desired_time = next_boundary_time + seconds(1);

    // If in the past, nudge forward by `interval` minutes
    if (desired_time <= now)
        desired_time += minutes(interval);

    // Convert the desired_time to a human-readable form for logs
    auto desired_time_t = system_clock::to_time_t(desired_time);
    auto desired_tm = *std::gmtime(&desired_time_t);

    // Determine the mode string based on the interval
    std::string mode_string = (interval == WSPR_2) ? "WSPR-2" : (interval == WSPR_15 ? "WSPR-15" : "Unknown mode");

    // Log the next transmission time
    std::ostringstream info_message;
    info_message << "Next transmission at "
                 << std::setw(2) << std::setfill('0') << desired_tm.tm_hour << ":"
                 << std::setw(2) << std::setfill('0') << desired_tm.tm_min << ":"
                 << "01 UTC (" << mode_string << ").";
    llog.logS(INFO, info_message.str());

    // **Sleep with periodic interrupt checks**
    while (system_clock::now() < desired_time)
    {
        if (exit_scheduler.load())
        {
            // Shutdown requested. Exiting scheduler.
            return false;
        }
        std::this_thread::sleep_for(milliseconds(100));  // Sleep in 100 ms chunks
    }

    // Capture the actual wake-up time
    auto final_time = system_clock::now();
    auto final_t = system_clock::to_time_t(final_time);
    auto final_tm = *std::gmtime(&final_t);

    // Calculate the offset in milliseconds from the desired target
    auto offset_ns = duration_cast<nanoseconds>(final_time - desired_time).count();
    double offset_ms = offset_ns / 1e6;  // Convert nanoseconds to milliseconds

    // Prepare offset message
    std::ostringstream offset_message;
    offset_message << "Offset from desired target: "
                   << (offset_ms >= 0 ? "+" : "-")
                   << std::fixed << std::setprecision(4) << std::abs(offset_ms) << " ms.";

    // Check if we hit the target interval
    if ((final_tm.tm_min % interval == 0) && final_tm.tm_sec >= 1)
    {
        llog.logS(INFO, "Transmission triggered at the target interval (" + std::to_string(interval) + " min).");
        llog.logS(DEBUG, offset_message.str());
        return true;
    }

    // Log an error if the interval was missed
    llog.logE(ERROR, "Missed target interval. " + offset_message.str());
    return false;
}

void scheduler_thread()
{
    if (setpriority(PRIO_PROCESS, 0, -10) == -1)
    {
        llog.logE(ERROR, "Failed to set scheduler thread priority:", std::string(strerror(errno)));
    }
    else
    {
        llog.logS(DEBUG, "Scheduler thread priority set.");
    }

    llog.logS(DEBUG, "Scheduler thread started.");
    std::unique_lock<std::mutex> lock(cv_mtx);

    while (!exit_scheduler)
    {
        // Check for CPU throttling every minute
        check_and_restore_governor();

        // Monitor for INI changes
        if (iniMonitor.changed())
        {
            llog.logS(INFO, "INI file changed. Reloading configuration.");
            validate_config_data();
        }

        // Read the current interval dynamically
        int current_interval = wspr_interval.load();
        llog.logS(DEBUG, "Current WSPR interval set to:", current_interval, "minutes.");

        // Wait for the next interval based on the updated value
        if (wait_every(current_interval))
        {
            transmit();
        }

        // Avoid busy-waiting with a conditional sleep
        cv.wait_for(lock, std::chrono::seconds(1), [] { return exit_scheduler.load(); });
    }

    llog.logS(DEBUG, "Scheduler thread exiting.");
}

void wspr_loop()
{
    while (!exit_scheduler.load())
    {
        exit_scheduler = false;  // Ensure clean state

        // Start scheduler thread
        active_threads.emplace_back(scheduler_thread);

        std::unique_lock<std::mutex> lock(cv_mtx);
        cv.wait(lock, [] { return exit_scheduler.load(); });

        // Join all threads
        for (auto &thread : active_threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
        active_threads.clear();

        if (!exit_scheduler.load())
        {
            transmit();  // Only transmit if shutdown wasn't requested
        }
    }
}

bool is_cpu_throttled()
{
    std::ifstream temp_file("/sys/class/thermal/thermal_zone0/temp");
    std::ifstream freq_file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");

    int temp = 0;
    int cur_freq = 0;

    if (temp_file.is_open())
    {
        temp_file >> temp;
        temp_file.close();
    }
    else
    {
        llog.logE(ERROR, "Failed to read CPU temperature.");
        return false;
    }

    if (freq_file.is_open())
    {
        freq_file >> cur_freq;
        freq_file.close();
    }
    else
    {
        llog.logE(ERROR, "Failed to read current CPU frequency.");
        return false;
    }

    // Check if the temperature is above 80°C (throttling threshold)
    bool temp_throttled = (temp > 80000);  // Temp is in millidegrees
    bool freq_throttled = (cur_freq < 1000000);  // Below 1 GHz usually indicates throttling

    if (temp_throttled || freq_throttled)
    {
        llog.logE(WARN, "CPU throttling detected! Temp:", temp / 1000, "°C\nCPU Frequency:", cur_freq / 1000, "MHz.");
        return true;
    }

    llog.logS(DEBUG, "CPU running normally. Temp:", temp / 1000, "°C\nCPU Frequency:", cur_freq / 1000, "MHz.");
    return false;
}

void check_and_restore_governor()
{
    if (is_cpu_throttled())
    {
        llog.logE(WARN, "CPU Throttling detected.");

        // Restore the previous governor if it was changed
        if (!config.previous_governor.empty() && config.previous_governor != "performance")
        {
            if (set_cpu_governor(config.previous_governor))
            {
                llog.logS(INFO, "CPU governor restored to:", config.previous_governor);
            }
            else
            {
                llog.logE(ERROR, "Failed to restore CPU governor.");
            }
        }
        else
        {
            llog.logE(WARN, "Governor was already 'performance' or not set.");
        }
    }
}
