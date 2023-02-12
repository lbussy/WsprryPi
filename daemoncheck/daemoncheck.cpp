#include <iostream>

// Invoke as WSPR_DAEMON=true ./getenv

bool isDaemon()
{
    return getenv ("WSPR_DAEMON");
}

int main ()
{
    if (isDaemon())
        std::cout << "Running as a daemon." << std::endl;
    else
        std::cout << "Not running as a daemon." << std::endl;
}
