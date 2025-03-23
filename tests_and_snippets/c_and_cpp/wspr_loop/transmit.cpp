// TODO:  Check Doxygen

// Primary header for this source file
#include "transmit.hpp"

#include "arg_parser.hpp"
#include "config_handler.hpp"
#include "constants.hpp"
#include "gpio_output.hpp"
#include "logging.hpp"
#include "scheduling.hpp"
#include "signal_handler.hpp"

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

void perform_transmission(ModeType mode, double tx_freq)
{

    // If TONE, transmit until break
    if (mode == ModeType::TONE)
    {
        // TODO: Transmit tone forever (till SIGNAL)
        while (true)
        {
            // Turn on LED
            ledControl.toggle_gpio(true);
            llog.logS(INFO, "Transmitting a test tone at",
                      lookup.freq_display_string(tx_freq));
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep for 100ms
        }
    }

    std::string mode_name = (wspr_interval.load() == WSPR_2) ? "WSPR" : "WSPR-15";
    llog.logS(INFO, "Transmission started for", mode_name, "at",
              lookup.freq_display_string(tx_freq));
    in_transmission.store(true);

    // Turn on LED
    ledControl.toggle_gpio(true);

    // TODO: This is a dummy value
    int duration = (wspr_interval.load() * 60) - (5 * wspr_interval.load());

    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + std::chrono::seconds(duration);

    while (std::chrono::steady_clock::now() < end_time)
    {
        // Monitor for shutdown while transmitting
        if (exit_wspr_loop.load())
        {
            llog.logS(WARN, "Transmission aborted due to shutdown.");
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Check every 100 ms
    }

    // Turn off LED
    ledControl.toggle_gpio(false);

    in_transmission.store(false);
    llog.logS(INFO, "Transmission ended.");
}

void transmit_loop()
{
    size_t freq_index = 0;
    int total_iterations = 0;

    while (!exit_wspr_loop.load())
    {
        auto now = std::chrono::system_clock::now();
        auto next_wakeup = now;

        int interval = wspr_interval.load();

        if (interval == WSPR_2)
        {
            int current_minute = std::chrono::duration_cast<std::chrono::minutes>(
                                     now.time_since_epoch())
                                     .count() %
                                 2;
            next_wakeup += std::chrono::minutes(current_minute == 0 ? 2 : 1);
            next_wakeup -= std::chrono::seconds(
                std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() % 60);
            next_wakeup += std::chrono::seconds(1);
        }
        else if (interval == WSPR_15)
        {
            int current_minute = std::chrono::duration_cast<std::chrono::minutes>(
                                     now.time_since_epoch())
                                     .count() %
                                 60;
            int next_target = ((current_minute / 15) + 1) * 15 % 60;
            next_wakeup += std::chrono::minutes(next_target - current_minute);
            next_wakeup -= std::chrono::seconds(
                std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() % 60);
            next_wakeup += std::chrono::seconds(1);
        }
        else
        {
            llog.logE(ERROR, "Invalid WSPR interval:", interval, "minutes. Retrying in 5s.");
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        // Ensure we wake up at the next transmission time or exit signal
        {
            std::unique_lock<std::mutex> lock(transmit_mtx);

            if (shutdown_cv.wait_until(lock, next_wakeup, []
                                       { return exit_wspr_loop.load(); }))
            {
                llog.logS(DEBUG, "Transmit loop wake-up due to shutdown request.");
                break;
            }
        }

        // Final shutdown check before transmission
        if (exit_wspr_loop.load())
        {
            break;
        }

        // Select transmission frequency
        if (config.center_freq_set.empty())
        {
            llog.logE(ERROR, "No frequencies available. Exiting transmit loop.");
            break;
        }

        double tx_freq = config.center_freq_set[freq_index];

        // ** TODO: PERFORM TRANSMISSION HERE**
        if (tx_freq != 0.0)
        {
            llog.logS(INFO, "Transmitting on frequency:", lookup.freq_display_string(tx_freq));
            perform_transmission(config.mode, tx_freq);
        }
        else
        {
            llog.logS(INFO, "Skipping transmission per 0.0 freq request in list.");
        }

        // Defered load if INI changes
        apply_deferred_changes();

        // Skip to next freq in list.
        freq_index = (freq_index + 1) % config.center_freq_set.size();

        // Check if we've reached the transmission limit
        if (!config.loop_tx)
        {
            total_iterations++;

            if (total_iterations >= (config.tx_iterations * static_cast<int>(config.center_freq_set.size())))
            {
                llog.logS(INFO, "Completed all scheduled transmissions. Signaling shutdown.");
                exit_wspr_loop.store(true); // Set exit flag
                shutdown_cv.notify_all();   // Wake up waiting threads
                break;
            }
        }
    }

    llog.logS(INFO, "Transmit loop exited.");
}
