#include "main.hpp"

#include <csignal>
#include <iostream>
#include <string.h>

void sig_handler(int sig = SIGTERM)
{
    // Called when exiting or when a signal is received.
    std::cout << "Caught signal: " << sig << " (" << strsignal(sig) << ").\n";
    dma_cleanup();
    exit(-1);
}

int main()
{
    // Catch all signals
    for (int i = 0; i < 64; i++)
    {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = sig_handler;
        sigaction(i, &sa, NULL);
    }

    atexit(dma_cleanup);

    setup_dma();
    tx_tone();

    return 0;
}
