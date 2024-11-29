#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void cleanupAndExit(int sig) {
    printf("Caught signal: %d\n", sig);
    exit(0);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = cleanupAndExit;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    // Set up SIGTERM handling
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Failed to set SIGTERM handler");
        return 1;
    }

    // Print the PID to send the SIGTERM signal to
    printf("PID: %d\n", getpid());

    printf("Waiting for SIGTERM...\n");

    // Main loop
    while (1) {
        sleep(1);
    }

    return 0;
}
