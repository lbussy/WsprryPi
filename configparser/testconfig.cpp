#include <unistd.h>
#include "config.h"
#include "singleton.hpp"

#define CONFIG_FILE "config.ini"
#define SINGLETON_PORT 1234

int main()
{
    SingletonProcess singleton(SINGLETON_PORT);
    if (!singleton())
    {
        std::cerr << "Process already running; see " << singleton.GetLockFileName() << std::endl;
        return 1;
    }

    WSPRConfig config(CONFIG_FILE);
    if (config.isInitialized())
    {
        std::cout << std::endl;
        std::cout << "Config loaded from:\t" << CONFIG_FILE << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "Transmit Enabled:\t" << config.getTransmit() << std::endl;
        std::cout << "Reset Detected:\t\t" << config.getreset() << std::endl;
        std::cout << "Call Sign:\t\t" << config.getCallsign() << std::endl;
        std::cout << "Grid Square:\t\t" << config.getGridsquare() << std::endl;
        std::cout << "Transmit Power:\t\t" << config.getTxpower() << std::endl;
        std::cout << "Frequency:\t\t" << config.getFrequency1() << std::endl;
        std::cout << "Added Frequency 2:\t" << config.getFrequency2() << std::endl;
        std::cout << "Added Frequency 3:\t" << config.getFrequency3() << std::endl;
        std::cout << "Added Frequency 4:\t" << config.getFrequency4() << std::endl;
        std::cout << "Added Frequency 5:\t" << config.getFrequency5() << std::endl;
        std::cout << "PPM Offset:\t\t" << config.getPpm() << std::endl;
        std::cout << "No Time Sync:\t\t" << config.getFreerun() << std::endl;
        std::cout << "Use Random Frequency:\t" << config.getOffset() << std::endl;
    }
    while (true)
    {
        sleep(1);
    }
}
