#include "wspr_transmit.hpp"

#include "utils.hpp"

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
    while (true)
    {
        int ch = getch();
        if (ch == ' ')
        {
            break;
        }
    }
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
    std::cout << "Caught signal: " << sig << " (" << strsignal(sig) << ").\n";
    g_stop.store(true);
    dma_cleanup();
    std::cout << "Cleaning stuff up." << std::endl;
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

    // Set high scheduling priority to reduce kernel interruptions
    setSchedPriority(30);

    // Initialize random number generator for transmission timing
    srand(time(nullptr));

    setup_dma();

    bool isWspr = select_wspr();
    std::cout << "Mode selected: " << (isWspr ? "WSPR" : "TONE") << std::endl;

    if (isWspr)
    {
        tx_wspr();
        dma_cleanup();
    }
    else
    {
        tx_tone();
        dma_cleanup();
    }

    return 0;
}
