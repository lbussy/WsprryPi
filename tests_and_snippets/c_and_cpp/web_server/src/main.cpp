#include "web_server.hpp"
#include <iostream>

constexpr const int PORT = 31415;

int main()
{
    try
    {
        // Create a global WebServer instance.
        WebServer server;

        // Start the server on a chosen port.
        server.start(PORT);

        std::cout << "Press Enter to stop the server." << std::endl;
        std::cin.get();

        // Stop the server cleanly.
        server.stop();
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Server error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
