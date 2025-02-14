#include <unistd.h>
#include "config.hpp"

#define CONFIG_FILE "config.ini"

int main()
{
    WSPRConfig config;
    if (config.initialize(CONFIG_FILE))
    {
        std::cout << std::endl;
        std::cout << "==========================================" << std::endl;
        std::cout << "Config loaded from: " << CONFIG_FILE << std::endl;
        std::cout << "==========================================" << std::endl;
        std::cout << "Transmit Enabled: " << std::boolalpha << config.getTransmit() << std::endl;
        std::cout << "Repeat Transmissions: " << std::boolalpha << config.getRepeat() << std::endl;
        std::cout << "Call Sign: " << config.getCallsign() << std::endl;
        std::cout << "Grid Square: " << config.getGridsquare() << std::endl;
        std::cout << "Transmit Power: " << config.getTxpower() << std::endl;
        std::cout << "Frequencies: " << config.getFrequency() << std::endl;
        std::cout << "PPM Offset: " << config.getPpm() << std::endl;
        std::cout << "Do not use NTP sync: " << std::boolalpha << (!config.getSelfcal()) << std::endl;
        std::cout << "Check NTP Each Run (default): " << std::boolalpha << config.getSelfcal() << std::endl;
        std::cout << "Use Frequency Randomization: " << std::boolalpha << config.getOffset() << std::endl;
        std::cout << "Use LED: " << std::boolalpha << config.getUseLED() << std::endl;
        std::cout << "==========================================" << std::endl;
    }
}
