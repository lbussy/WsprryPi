#include <unistd.h>
#include "monitorfile.hpp"

#define FILE_NAME "./monitorfile.ini"

int main()
{
    MonitorFile monitor(FILE_NAME);

    while (true)
    {
        if (monitor.changed())
        {
            std::cout << "File changed." << std::endl;
        }
        sleep(1);
    }
}
