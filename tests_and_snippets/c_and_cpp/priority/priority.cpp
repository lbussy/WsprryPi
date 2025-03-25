// TODO: Handle doxygen

#include "priority.hpp"

#include "lcblog.hpp"
#include "logging.hpp"

#include <filesystem>
#include <fstream>
#include <vector>

#include <string.h>
#include <sys/resource.h>

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
    if (!all_cpu_cores.empty())
        return;

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
        return false; // Cannot determine throttling without a valid threshold.
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
        return false; // Assume no throttling if temperature cannot be checked.
    }

    // Track throttling status.
    bool throttled = false;

    // Check each CPU core's current frequency.
    for (const auto &core : all_cpu_cores)
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
    constexpr int kMaxSafeTempMilliC = 80000; // 80°C threshold.
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
    sp.sched_priority = 75; // Mid-high priority (range: 1–99).

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
