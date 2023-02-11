#include <unistd.h>
#include "config.hpp"

#define CONFIG_FILE "config.ini"

int main()
{
    WSPRConfig config(CONFIG_FILE);
    if (config.isInitialized())
    {
        std::cout << std::endl;
        std::cout << "Config loaded from: " << CONFIG_FILE << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "Transmit Enabled:\t\t" << config.getTransmit() << std::endl;
        std::cout << "Call Sign:\t\t\t" << config.getCallsign() << std::endl;
        std::cout << "Grid Square:\t\t\t" << config.getGridsquare() << std::endl;
        std::cout << "Transmit Power:\t\t\t" << config.getTxpower() << std::endl;
        std::cout << "Frequencies:\t\t\t" << config.getFrequency() << std::endl;
        std::cout << "PPM Offset:\t\t\t" << config.getPpm() << std::endl;
        std::cout << "Do not use NTP sync:\t\t" << (!config.getSelfcal()) << std::endl;
        std::cout << "Check NTP Each Run (default):\t" << config.getSelfcal() << std::endl;
        std::cout << "Use Frequency Randomization:\t" << config.getOffset() << std::endl;
        std::cout << "Use LED:\t\t\t" << config.useLED() << std::endl;
        std::cout << "Use daemon mode:\t\t" << config.useDaemon() << std::endl;
    }
}
