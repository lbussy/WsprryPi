#include <iostream>
#include "singleton.hpp"

#define SINGLETON_PORT 1234

int main()
{
    SingletonProcess singleton(SINGLETON_PORT);
    if (!singleton())
    {
        std::cerr << "Process already running on port " << singleton.GetLockFileName() << std::endl;
        return 1;
    }
    std::cout << "Running on port " << SINGLETON_PORT << "." << std::endl;
    while (true)
    {;}
}
