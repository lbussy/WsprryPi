#include <unistd.h>
#include "config.hpp"

#define CONFIG_FILE "config.ini"

int main()
{
    WSPRConfig config(CONFIG_FILE);
    if (config.isInitialized())
    {
        std::cout << std::endl;
        std::cout << "Config loaded from:\t" << CONFIG_FILE << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "Transmit Enabled:\t" << config.getTransmit() << std::endl;
        std::cout << "Call Sign:\t\t" << config.getCallsign() << std::endl;
        std::cout << "Grid Square:\t\t" << config.getGridsquare() << std::endl;
        std::cout << "Transmit Power:\t\t" << config.getTxpower() << std::endl;
        std::cout << "Frequency:\t\t" << config.getFrequency() << std::endl;
        std::cout << "PPM Offset:\t\t" << config.getPpm() << std::endl;
        std::cout << "No Time Sync:\t\t" << config.getFreerun() << std::endl;
        std::cout << "Use Random Frequency:\t" << config.getOffset() << std::endl;
    }
}
