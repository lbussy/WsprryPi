#include <unistd.h>
#include "monitorfile.hpp"

#define FILE_NAME "./monitorfile.ini"

MonitorFile monitor;

int main()
{
    monitor.filemon(FILE_NAME);
    while (true)
    {
        if (monitor.changed())
        {
            std::cout << "File changed." << std::endl;
        }
        sleep(1);
    }
}
