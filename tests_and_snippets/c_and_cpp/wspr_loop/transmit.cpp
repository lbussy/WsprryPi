#include "transmit.hpp"

#include <iomanip>
#include <time.h>
#include <sys/resource.h>
#include <string.h>

#include "scheduling.hpp"
#include "arg_parser.hpp"

void transmit()
{
    if (exit_scheduler.load())
    {
        llog.logS(WARN, "Shutdown requested. Skipping transmission.");
        return;
    }

    set_transmission_realtime();  // Ensure real-time priority for transmission
    llog.logS(INFO, "Transmission started.");

    // Record the start time
    timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Target time for the end of transmission
    timespec target_time = start_time;
    target_time.tv_sec += 110;  // 110 seconds of transmission

    // Sleep in short intervals to allow interruption
    while (true)
    {
        if (exit_scheduler.load())
        {
            llog.logS(WARN, "Shutdown requested. Aborting transmission.");
            break;  // Exit early if shutdown is requested
        }

        // Calculate remaining time until the target
        timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        if ((now.tv_sec > target_time.tv_sec) ||
            (now.tv_sec == target_time.tv_sec && now.tv_nsec >= target_time.tv_nsec))
        {
            break;  // Transmission time completed
        }

        // Sleep for a short interval (100 ms) and recheck
        timespec sleep_interval = {0, 100'000'000};  // 100 ms in nanoseconds
        clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep_interval, nullptr);
    }

    // Record the end time, even if interrupted
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    // Calculate actual duration, considering early exit
    double actual_duration = (end_time.tv_sec - start_time.tv_sec) +
                             (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    // Log the result, whether completed or interrupted
    if (exit_scheduler.load())
    {
        llog.logS(WARN, "Transmission aborted after", std::fixed, std::setprecision(6), actual_duration, "seconds.");
    }
    else
    {
        llog.logS(INFO, std::fixed, std::setprecision(6), "Transmission complete:", actual_duration, "seconds.");
    }

    // Reset priority after transmission
    if (setpriority(PRIO_PROCESS, 0, 0) == -1)
    {
        llog.logE(ERROR, "Failed to reset transmission priority:", std::string(strerror(errno)));
    }
    else
    {
        llog.logS(DEBUG, "Transmission priority reset to normal.");
    }
}
