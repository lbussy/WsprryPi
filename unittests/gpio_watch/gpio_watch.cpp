#include <pigpio.h>
#include <iostream>
#include <cstdlib>
#include <csignal>

#define WATCH_PIN 17 // TAPR shutdown button is on Pin 35, GPIO19, BCM19

volatile bool running = true;

void handleSignal(int signal) {
    running = false;
    std::cout << "\nExiting program...\n";
}

int main() {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    if (gpioInitialise() < 0) {
        std::cerr << "Failed to initialize pigpio." << std::endl;
        return 1;
    }

    gpioSetMode(WATCH_PIN, PI_INPUT);
    gpioSetPullUpDown(WATCH_PIN, PI_PUD_UP);

    std::cout << "Monitoring GPIO pin " << WATCH_PIN << " for pull-low condition..." << std::endl;

    while (running) {
        if (gpioRead(WATCH_PIN) == PI_LOW) {
            std::cout << "GPIO " << WATCH_PIN << " pulled LOW. Rebooting system...\n";
            system("sudo reboot");
            break;
        }
        time_sleep(0.01); // Poll every 10ms
    }

    gpioTerminate();
    return 0;
}
