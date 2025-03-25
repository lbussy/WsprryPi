// TODO: Handle doxygen

#ifndef _PRIORITY_HPP
#define _PRIORITY_HPP

#include <string>

/**
 * @brief Checks if the directory name represents a real CPU directory (e.g. "cpu0").
 *
 * We do a quick check:
 * - Must start with "cpu"
 * - Must have at least 4 characters ("cpu0" is the shortest)
 * - All characters after "cpu" must be digits.
 */
extern bool is_cpu_directory(const std::string &dirName);

/**
 * @brief Populates the list of valid CPU core directories.
 *
 * This function scans `/sys/devices/system/cpu/` and stores directory names like
 * "cpu0", "cpu1", etc. in the global `all_cpu_cores` vector for later use.
 */
extern void discover_cpu_cores();

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
extern bool is_cpu_throttled();

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
extern void set_transmission_realtime();

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
extern void set_scheduling_priority();

#endif // _PRIORITY_HPP
