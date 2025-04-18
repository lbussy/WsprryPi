// Project Headers
#include "config_handler.hpp"
#include "utils.hpp"
#include "wspr_transmit.hpp"

// Submodule Headers

// Standard C++ Headers
#include <atomic>
#include <array>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <csignal>
#include <ctime>
#include <iostream>
#include <thread>

#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <sys/select.h>

/**
 * @brief Global configuration object.
 *
 * This ArgParserConfig instance holds the application’s configuration settings,
 * typically loaded from an INI file or a JSON configuration.
 */
ArgParserConfig config;

/**
 * @brief Reads a single character from standard input without waiting for Enter.
 *
 * This function configures the terminal for noncanonical mode to read a single
 * character immediately. It then restores the terminal settings.
 *
 * @return The character read from standard input.
 */
int getch()
{
    struct termios oldAttr, newAttr;
    tcgetattr(STDIN_FILENO, &oldAttr); // get terminal attributes
    newAttr = oldAttr;
    newAttr.c_lflag &= ~(ICANON | ECHO);        // disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newAttr); // set new terminal attributes
    int ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldAttr); // restore old terminal attributes
    return ch;
}

/**
 * @brief Pauses the program until the user presses the spacebar.
 */
void pause_for_space()
{
    extern WsprTransmitter wsprTransmitter;

    while (!wsprTransmitter.is_stopping() && getch() != ' ')
        ; // spin until stop or spacebar
}

bool select_wspr()
{
    std::string input;

    std::cout << "Select mode:\n";
    std::cout << "  1) WSPR\n";
    std::cout << "  2) TONE\n";
    std::cout << "Enter choice [1-2, default 1]: ";
    std::getline(std::cin, input);

    int choice = 1;
    try
    {
        if (!input.empty())
        {
            choice = std::stoi(input);
        }
    }
    catch (...)
    {
        choice = 1; // fallback to default
    }
    return (choice == 1);
}

void sig_handler(int sig = SIGTERM)
{
    // Called when exiting or when a signal is received.
    std::cout << "Caught signal: " << sig << " (" << strsignal(sig) << ")." << std::endl;
    g_stop.store(true);
    std::cout << "Shutting down transmissions." << std::endl;
    wsprTransmitter.shutdown_transmitter();
    std::cout << "Reset DMA." << std::endl;
    wsprTransmitter.dma_cleanup();
    exit(EXIT_SUCCESS);
}

int main()
{
    std::array<int, 6> signals = {SIGINT, SIGTERM, SIGHUP, SIGUSR1, SIGUSR2, SIGQUIT};
    for (int sig : signals)
    {
        struct sigaction sa{};
        sa.sa_handler = sig_handler;
        sigaction(sig, &sa, nullptr);
    }
    std::signal(SIGCHLD, SIG_IGN);

    bool isWspr = select_wspr();
    std::cout << "Mode selected: " << (isWspr ? "WSPR" : "TONE") << std::endl;

    // Get adjustments based on PPM
    config.ppm = get_ppm_from_chronyc();

    if (isWspr)
    {
        wsprTransmitter.setup_transmission(7040100.0, 0, config.ppm, "AA0NT", "EM18", 20, true);
    }
    else
    {
        wsprTransmitter.setup_transmission(7040100.0, 0, config.ppm);
    }

    // Print transmission parameters
    wsprTransmitter.print_parameters();

    std::cout << "Setup for " << (isWspr ? "WSPR" : "tone") << " complete." << std::endl;
    if (isWspr)
    {
        std::cout << "Waiting for next transmission window." << std::endl;
        std::cout << "Press <spacebar> to start immediately." << std::endl;
        wait_for_trans_window();
    }
    else
    {
        std::cout << "Press <spacebar> to begin transmission." << std::endl;
        pause_for_space();
    }

    // Time structs for computing transmission window
    struct timeval tv_begin, tv_end, tv_diff;
    gettimeofday(&tv_begin, NULL);
    std::cout << "TX started at: " << timeval_print(&tv_begin) << std::endl;

    // Execute transmission
    wsprTransmitter.start_threaded_transmission(SCHED_FIFO, 30);

    if (isWspr)
    {
        // Now wait for the transmit thread to exit:
        wsprTransmitter.join_transmission();
    }
    else
    {
        std::cout << "Press <spacebar> to end." << std::endl;
        pause_for_space(); // Or some other user action
        std::cout << "Stopping." << std::endl;
    }

    // Print transmission timestamp and duration
    gettimeofday(&tv_end, nullptr);
    timeval_subtract(&tv_diff, &tv_end, &tv_begin);
    std::cout << "TX ended at: " << timeval_print(&tv_end)
              << " (" << tv_diff.tv_sec << "." << std::setfill('0') << std::setw(3)
              << (tv_diff.tv_usec + 500) / 1000 << " seconds)" << std::endl;

    // Get adjustments based on PPM
    // config.ppm = get_ppm_from_chronyc();
    // Update with:
    // update_dma_for_ppm(config.ppm);

    wsprTransmitter.shutdown_transmitter();
    wsprTransmitter.dma_cleanup();

    return 0;
}
