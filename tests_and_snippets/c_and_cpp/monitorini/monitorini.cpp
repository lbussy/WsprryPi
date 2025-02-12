#include <unistd.h>
#include "config.hpp"
#include "monitorfile.hpp"

#define CONFIG_FILE "config.ini"

std::string inifile = CONFIG_FILE;
WSPRConfig config;
MonitorFile monitor;

void getConfig(std::string inifile)
{
    if (config.initialize(inifile))
    {
        std::cout << std::endl;
        std::cout << "Config loaded from: " << inifile << std::endl;
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
    }
}

int main()
{
    monitor.filemon(inifile);
    std::cout << "Monitoring: " << inifile << "." << std::endl;
    while (true)
    {
        if (monitor.changed())
        {
            getConfig(inifile);
        }
        sleep(1);
    }
}
