// TODO:  Check Doxygen

// Primary header for this source file
#include "transmit.hpp"

#include "constants.hpp"
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
#include <vector>
#include <csignal>

std::atomic<bool> in_transmission(false);
std::thread transmit_thread;
std::mutex transmit_mtx;
std::vector<double> center_freq_set = {}; ///< Vector of frequencies in Hz
std::optional<int> tx_iterations;         ///< Number of transmissions before termination (if set)
bool loop_tx = false;                     ///< Flag to enable repeated transmission cycles.
float test_tone = 0.0;                    ///< Frequency for test tone mode.
ModeType mode = ModeType::WSPR;           ///< Current operating mode.

void perform_transmission(ModeType mode, double tx_freq)
{

    // If TONE, transmit until break
    if (mode == ModeType::TONE)
    {
        // TODO: Transmit tone forever (till SIGNAL)
        while (true)
        {
            // Turn on LED
            if (led_handler)
                toggle_led(true);
            llog.logS(INFO, "Transmitting a test tone at",
                      std::fixed, std::setprecision(6), tx_freq / 1e6, "MHz.");
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep for 100ms
        }
    }

    std::string mode_name = (wspr_interval.load()  == WSPR_2) ? "WSPR" : "WSPR-15";
    llog.logS(INFO, "Transmission started for", mode_name, "at",
              std::fixed, std::setprecision(6), tx_freq / 1e6, "MHz.");
    in_transmission.store(true);

    // Turn on LED
    if (led_handler)
        toggle_led(true);

    // TODO: This is a dummy value
    int duration = (wspr_interval.load() * 60) - (5 * wspr_interval.load());

    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + std::chrono::seconds(duration);

    while (std::chrono::steady_clock::now() < end_time)
    {
        // Monitor for shutdown while transmitting
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
    size_t freq_index = 0;
    int total_iterations = 0;

    while (!exit_wspr_loop.load() && !signal_shutdown.load())
    {
        auto now = std::chrono::system_clock::now();
        auto next_wakeup = now;

        int interval = wspr_interval.load();

        if (interval == WSPR_2)
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
            next_wakeup += std::chrono::seconds(1);
        }
        else if (interval == WSPR_15)
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
            next_wakeup += std::chrono::seconds(1);
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
            break;
        }

        // Shutdown check before transmission.
        if (exit_wspr_loop.load() || signal_shutdown.load())
        {
            break;
        }

        // Fetch the current transmission frequency.
        if (center_freq_set.empty())
        {
            llog.logE(ERROR, "No frequencies in frequency set. Exiting transmit loop.");
            break;
        }

        double tx_freq = center_freq_set[freq_index];

        // Skip transmission if the frequency is 0.0
        if (tx_freq != 0.0)
        {
            if (interval == WSPR_2)
            {
                perform_transmission(mode, tx_freq); // 2-minute interval = 110 seconds transmission.
            }
            else if (interval == WSPR_15)
            {
                perform_transmission(mode, tx_freq); // 15-minute interval = 885 seconds transmission.
            }
        }
        else
        {
            llog.logS(INFO, "Skipping transmission.");
        }

        // Move to the next frequency in the list.
        freq_index = (freq_index + 1) % center_freq_set.size();

        // Increment transmission counter if not in loop mode.
        if (!loop_tx)
        {
            total_iterations++;

            // Check full iterations of center_freq_set against tx_iterations
            size_t iterations = static_cast<size_t>(tx_iterations.value_or(1)); // Convert to size_t

            if (static_cast<size_t>(total_iterations) >= (iterations * center_freq_set.size()))
            {
                llog.logS(INFO, "Completed all scheduled transmissions. Exiting.");
                signal_handler(SIGTERM);
                return;
            }
        }
    }

    llog.logS(INFO, "Transmit loop exiting.");
}
