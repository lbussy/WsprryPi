#include "wspr_transmit.hpp"

#include <chrono>
#include <csignal>
#include <ctime>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <thread>
#include <sys/select.h>

/**
 * @brief Reads a single character from standard input without waiting for Enter.
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
        // TODO: Setup

        // std::cout << "Setup complete. Waiting until one second after the next even minute\n"
        //           << "or press SPACE to transmit immediately." << std::endl;
        // wait_until_next_even_minute_plus_one_second_or_space();

        tx_wspr();

        dma_cleanup();
    }
    else
    {
        // TODO: Setup

        std::cout << "Setup complete, hit spacebar to transmit TONE." << std::endl;
        pause_for_space();
        // TODO: Transmit

        dma_cleanup();
    }

    return 0;
}
