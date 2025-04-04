#include "wspr_transmit.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <csignal>
#include <ctime>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <thread>
#include <sys/select.h>

// Global stop flag.
std::atomic<bool> g_stop{false};

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
    newAttr.c_lflag &= ~(ICANON | ECHO); // disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newAttr); // set new terminal attributes
    int ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldAttr); // restore old terminal attributes
    return ch;
}

/**
 * @brief Waits until it is exactly one second past an even minute, until spacebar is pressed, or a stop is requested.
 *
 * This function sets the terminal to noncanonical mode so that a single spacebar press is immediately
 * detected (without requiring Enter). It then periodically checks the system clock and uses select() with
 * a timeout to wait until the next second boundary. If the spacebar is pressed, the function exits early.
 *
 * @note Due to system scheduling, the wake-up time might not be exactly at the second boundary.
 */
void waitForOneSecondPastEvenMinute() {
    // Set terminal to noncanonical mode to capture key presses immediately.
    struct termios oldAttr, newAttr;
    tcgetattr(STDIN_FILENO, &oldAttr);
    newAttr = oldAttr;
    newAttr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newAttr);

    std::mutex m;
    std::condition_variable cv;
    std::unique_lock<std::mutex> lock(m);

    while (!g_stop.load()) {
        // Get the current system time.
        auto now = std::chrono::system_clock::now();
        std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now = *std::localtime(&now_time_t);

        // Check if the minute is even and the seconds count is exactly 1.
        if ((tm_now.tm_min % 2 == 0) && (tm_now.tm_sec == 1)) {
            std::cout << "Condition met: One second past an even minute." << std::endl;
            break;
        }

        // Calculate the next second boundary.
        auto next_second = now + std::chrono::seconds(1);
        auto wait_duration = std::chrono::duration_cast<std::chrono::microseconds>(next_second - now);

        // Prepare the timeout for select.
        struct timeval tv;
        tv.tv_sec = wait_duration.count() / 1000000;
        tv.tv_usec = wait_duration.count() % 1000000;

        // Set up file descriptor set for standard input.
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        // Use select to wait until either input is available or the timeout expires.
        int ret = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
            // Input is available; read one character.
            char c;
            ssize_t n = read(STDIN_FILENO, &c, 1);
            if (n > 0 && c == ' ') {
                std::cout << "Spacebar pressed. Exiting wait early." << std::endl;
                break;
            }
        }
        // If ret == 0, the timeout expired and the loop continues.
    }

    // Restore terminal settings.
    tcsetattr(STDIN_FILENO, TCSANOW, &oldAttr);
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

bool select_wspr() {
    std::string input;

    std::cout << "Select mode:\n";
    std::cout << "  1) WSPR\n";
    std::cout << "  2) TONE\n";
    std::cout << "Enter choice [1-2, default 1]: ";
    std::getline(std::cin, input);

    int choice = 1;
    try {
        if (!input.empty()) {
            choice = std::stoi(input);
        }
    } catch (...) {
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
    // Catch all signals.
    for (int i = 0; i < 64; i++)
    {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = sig_handler;
        sigaction(i, &sa, NULL);
    }

    setup_dma();

    bool isWspr = select_wspr();
    std::cout <<  "Mode selected: " << (isWspr ? "WSPR" : "TONE") << std::endl;

    if (isWspr)
    {
        // TODO: Setup before waiting
        std::cout << "Waiting for next transmission window." << std::endl;
        std::cout << "Press <spacebar> to start immediately." << std::endl;

        waitForOneSecondPastEvenMinute();

        tx_wspr();

        dma_cleanup();
    }
    else
    {
        // TODO: Setup before waiting

        std::cout << "Setup complete, hit spacebar to transmit TONE." << std::endl;
        pause_for_space();
        // TODO: Transmit

        dma_cleanup();
    }

    return 0;
}
