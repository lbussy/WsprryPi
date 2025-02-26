// TODO:  Check Doxygen

// Primary header for this source file
#include "transmit.hpp"

#include "gpio_handler.hpp"
#include "logging.hpp"
#include "scheduling.hpp"
#include "signal_handler.hpp"
#include "arg_parser.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>

std::atomic<bool> in_transmission(false);
std::thread transmit_thread;
std::mutex transmit_mtx;

void perform_transmission(int duration)
{
    llog.logS(INFO, "Transmission started for", duration, "seconds.");
    in_transmission.store(true);

    // Turn on LED
    if (led_handler)
        toggle_led(true);

    // Monitor for shutdown while transmitting
    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + std::chrono::seconds(duration);

    while (std::chrono::steady_clock::now() < end_time)
    {
        if (exit_wspr_loop.load() || signal_shutdown.load())
        {
            llog.logS(WARN, "Transmission aborted due to shutdown.");
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Check every 100 ms
    }

    // Turn off LED
    if (led_handler)
        toggle_led(false);

    in_transmission.store(false);
    llog.logS(INFO, "Transmission ended.");
}

void transmit_loop()
{
    while (!exit_wspr_loop.load() && !signal_shutdown.load())
    {
        auto now = std::chrono::system_clock::now();
        auto next_wakeup = now;

        int interval = wspr_interval.load();

        if (interval == 2)
        {
            // Wake every even minute at one second past.
            int current_minute = std::chrono::duration_cast<std::chrono::minutes>(
                                     now.time_since_epoch())
                                     .count() %
                                 2;
            next_wakeup += std::chrono::minutes(current_minute == 0 ? 2 : 1);
            next_wakeup -= std::chrono::seconds(
                std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() %
                60);
            next_wakeup += std::chrono::seconds(1); // 1 second past the minute.
        }
        else if (interval == 15)
        {
            // Wake at 0, 15, 30, 45 minutes.
            int current_minute = std::chrono::duration_cast<std::chrono::minutes>(
                                     now.time_since_epoch())
                                     .count() %
                                 60;
            int next_target = ((current_minute / 15) + 1) * 15 % 60;
            next_wakeup += std::chrono::minutes(next_target - current_minute);
            next_wakeup -= std::chrono::seconds(
                std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() %
                60);
            next_wakeup += std::chrono::seconds(1); // 1 second past the minute.
        }
        else
        {
            llog.logE(ERROR, "Invalid WSPR interval:", interval, "minutes.");
            std::this_thread::sleep_for(std::chrono::minutes(1));
            continue;
        }

        // Sleep until the next transmission time or shutdown signal.
        std::unique_lock<std::mutex> lock(transmit_mtx);
        if (cv.wait_until(lock, next_wakeup, []
                          { return exit_wspr_loop.load() || signal_shutdown.load(); }))
        {
            break; // Exit loop if shutdown is requested.
        }

        // Perform the placeholder event based on the interval.
        if (!exit_wspr_loop.load() && !signal_shutdown.load())
        {
            if (interval == 2)
            {
                perform_transmission(110); // 2-minute interval = 110 seconds transmission.
            }
            else if (interval == 15)
            {
                perform_transmission(885); // 15-minute interval = 885 seconds transmission.
            }
        }
    }

    llog.logS(INFO, "Transmit loop exiting.");
}
