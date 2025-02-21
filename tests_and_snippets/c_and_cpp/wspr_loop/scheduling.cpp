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
#include <regex>

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
    sp.sched_priority = 75; // mid-high range (1–99)

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

bool get_current_governors()
{
    config.previous_governor.clear();
    bool success = false;

    for (const auto &entry : std::filesystem::directory_iterator("/sys/devices/system/cpu/"))
    {
        if (!entry.is_directory())
            continue;

        std::string dirName = entry.path().filename().string();
        // Only handle directories like "cpu0", "cpu1", etc.
        if (!std::regex_match(dirName, std::regex("^cpu[0-9]+$")))
        {
            // skip cpufreq, cpuidle, etc.
            continue;
        }

        // Real CPU -> read scaling_governor
        std::string governor_path = entry.path().string() + "/cpufreq/scaling_governor";
        if (std::filesystem::exists(governor_path))
        {
            std::ifstream file(governor_path);
            if (file.is_open())
            {
                std::string governor;
                file >> governor;
                file.close();
                config.previous_governor[dirName] = governor;
                success = true;
            }
        }
    }

    return success;
}

bool set_cpu_governor_for_core(const std::string &core, const std::string &governor)
{
    // e.g. core = "cpu0"
    std::string cpufreq_path = "/sys/devices/system/cpu/" + core + "/cpufreq";
    if (!std::filesystem::exists(cpufreq_path))
    {
        llog.logE(WARN, "cpufreq directory not found for:", core);
        return false;
    }

    std::string governor_path = cpufreq_path + "/scaling_governor";
    std::ofstream file(governor_path);
    if (!file.is_open())
    {
        llog.logE(ERROR, "Failed to set CPU governor to:", governor, "for", core);
        return false;
    }

    file << governor;
    file.close();
    llog.logS(DEBUG, "CPU governor set to:", governor, "for", core);
    return true;
}

bool set_cpu_governor(const std::string &governor)
{
    bool success = true;

    for (const auto &entry : std::filesystem::directory_iterator("/sys/devices/system/cpu/"))
    {
        if (!entry.is_directory())
            continue;

        std::string dirName = entry.path().filename().string();

        // Only proceed if this is a real CPU directory like "cpu0", "cpu1", etc.
        if (!std::regex_match(dirName, std::regex("^cpu[0-9]+$")))
            continue;

        // cpufreq path for this CPU
        std::string cpufreq_path = entry.path().string() + "/cpufreq";
        if (!std::filesystem::exists(cpufreq_path))
        {
            llog.logE(WARN, "cpufreq directory not found for:", dirName);
            continue;
        }

        // Attempt to set scaling_governor
        std::string governor_path = cpufreq_path + "/scaling_governor";
        std::ofstream file(governor_path);
        if (file.is_open())
        {
            file << governor;
            file.close();
            llog.logS(DEBUG, "CPU governor set to:", governor, "for", dirName);
        }
        else
        {
            llog.logE(ERROR, "Failed to set CPU governor to:", governor, "for", dirName);
            success = false;
        }
    }

    if (!success)
    {
        llog.logE(ERROR, "One or more CPU cores failed to set the governor to:", governor);
    }

    return success;
}

void enable_performance_mode()
{
    // 1) Refresh map of previous governors
    bool success = get_current_governors();
    if (!success)
    {
        llog.logE(ERROR, "Failed to retrieve current CPU governors.");
        return;
    }

    // 2) If map is empty, there's no CPU info
    if (config.previous_governor.empty())
    {
        llog.logE(ERROR, "No CPU governors found.");
        return;
    }

    // 3) Check if all are already performance
    bool all_in_performance = true;
    for (const auto &entry : config.previous_governor)
    {
        if (entry.second != "performance")
        {
            all_in_performance = false;
            break;
        }
    }

    // 4) If not all performance, set them
    if (!all_in_performance)
    {
        if (set_cpu_governor("performance"))
        {
            llog.logS(INFO, "Performance mode enabled for all CPU cores.");
        }
        else
        {
            llog.logE(ERROR, "Failed to enable performance mode on all cores.");
        }
    }
    else
    {
        llog.logS(DEBUG, "All CPU cores are already in performance mode.");
    }
}

void restore_governor()
{
    // Make sure we have stored data
    if (config.previous_governor.empty())
    {
        llog.logE(ERROR, "No previous CPU governor settings found.");
        return;
    }

    bool success = true;

    // Iterate the map: (cpu0 -> ondemand, cpu1 -> performance, etc.)
    for (const auto &entry : config.previous_governor)
    {
        const std::string &core = entry.first;
        const std::string &governor = entry.second;

        // Use the single-core function:
        if (!set_cpu_governor_for_core(core, governor))
        {
            llog.logE(ERROR, "Failed to restore governor for", core, "to", governor);
            success = false;
        }
        else
        {
            llog.logS(DEBUG, "Restored governor for", core, "to", governor);
        }
    }

    if (success)
    {
        llog.logS(INFO, "All CPU governors restored successfully.");
    }
    else
    {
        llog.logE(ERROR, "Failed to restore some CPU governors.");
    }
}

void precise_sleep_until(const timespec &target)
{
    while (true)
    {
        int res = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &target, nullptr);
        if (res == 0) break;  // Success
        if (res != EINTR)
        {
            llog.logE(ERROR, "clock_nanosleep failed:", std::string(strerror(res)));
            break;
        }
        // If interrupted, keep trying until target
    }
}

bool wait_every(int interval)
{
    using namespace std::chrono;
    auto now = system_clock::now();

    // Convert current time to integral minutes since epoch
    auto current_min_time = time_point_cast<minutes>(now);
    auto total_minutes = duration_cast<minutes>(current_min_time.time_since_epoch()).count();

    // Next boundary minute
    auto next_boundary = ((total_minutes / interval) + 1) * interval;
    auto next_boundary_time = system_clock::time_point{minutes(next_boundary)};
    auto desired_time = next_boundary_time + seconds(1);

    if (desired_time <= now)
        desired_time += minutes(interval);

    auto desired_time_t = system_clock::to_time_t(desired_time);
    auto desired_tm = *std::gmtime(&desired_time_t);

    std::string mode_string = (interval == WSPR_2) ? "WSPR-2"
                           : (interval == WSPR_15) ? "WSPR-15"
                                                   : "Unknown mode";

    std::ostringstream info_message;
    info_message << "Next transmission at "
                 << std::setw(2) << std::setfill('0') << desired_tm.tm_hour << ":"
                 << std::setw(2) << std::setfill('0') << desired_tm.tm_min << ":"
                 << "01 UTC (" << mode_string << ").";
    llog.logS(INFO, info_message.str());

    // Sleep until desired_time
    while (system_clock::now() < desired_time)
    {
        if (exit_scheduler.load()) return false;
        std::this_thread::sleep_for(milliseconds(100));
    }

    // Actual wake-up
    auto final_time = system_clock::now();
    auto final_t = system_clock::to_time_t(final_time);
    auto final_tm = *std::gmtime(&final_t);

    auto offset_ns = duration_cast<nanoseconds>(final_time - desired_time).count();
    double offset_ms = offset_ns / 1e6;

    std::ostringstream offset_message;
    offset_message << "Offset from desired target: "
                   << (offset_ms >= 0 ? "+" : "-")
                   << std::fixed << std::setprecision(4)
                   << std::abs(offset_ms) << " ms.";

    if ((final_tm.tm_min % interval == 0) && final_tm.tm_sec >= 1)
    {
        llog.logS(INFO, "Transmission triggered at the target interval (" + std::to_string(interval) + " min).");
        llog.logS(DEBUG, offset_message.str());
        return true;
    }

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
        // Periodically check throttling
        check_and_restore_governor();

        // Monitor for INI changes
        if (iniMonitor.changed())
        {
            llog.logS(INFO, "INI file changed. Reloading configuration.");
            validate_config_data();
        }

        // WSPR interval
        int current_interval = wspr_interval.load();
        llog.logS(DEBUG, "Current WSPR interval set to:", current_interval, "minutes.");

        if (wait_every(current_interval))
        {
            transmit();
        }

        cv.wait_for(lock, std::chrono::seconds(1), [] { return exit_scheduler.load(); });
    }

    llog.logS(DEBUG, "Scheduler thread exiting.");
}

void wspr_loop()
{
    while (!exit_scheduler.load())
    {
        exit_scheduler = false;

        // Start scheduler
        active_threads.emplace_back(scheduler_thread);

        std::unique_lock<std::mutex> lock(cv_mtx);
        cv.wait(lock, [] { return exit_scheduler.load(); });

        // Join all
        for (auto &thread : active_threads)
        {
            if (thread.joinable()) thread.join();
        }
        active_threads.clear();

        if (!exit_scheduler.load())
        {
            transmit();
        }
    }
}

bool is_cpu_throttled()
{
    std::ifstream temp_file("/sys/class/thermal/thermal_zone0/temp");
    std::ifstream freq_file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");

    int temp = 0;
    int cur_freq = 0;

    if (temp_file.is_open()) {
        temp_file >> temp;
        temp_file.close();
    } else {
        llog.logE(ERROR, "Failed to read CPU temperature.");
        return false;
    }

    if (freq_file.is_open()) {
        freq_file >> cur_freq;
        freq_file.close();
    } else {
        llog.logE(ERROR, "Failed to read current CPU frequency.");
        return false;
    }

    bool temp_throttled = (temp > 80000);        // > 80°C
    bool freq_throttled = (cur_freq < 1000000);  // < 1GHz

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

        // Instead of comparing a map to "performance", check if *all* are "performance"
        bool all_perf = true;
        for (auto &kv : config.previous_governor)
        {
            if (kv.second != "performance")
            {
                all_perf = false;
                break;
            }
        }

        // If there's at least one non-performance, attempt to restore
        if (!config.previous_governor.empty() && !all_perf)
        {
            // We want to revert them all to their old settings:
            //   In your original code, you did `set_cpu_governor(config.previous_governor)`
            //   which doesn't exist. Instead, let's loop or call restore_governor():
            restore_governor();
        }
        else
        {
            llog.logE(WARN, "Governor was already 'performance' or map was empty.");
        }
    }
}
