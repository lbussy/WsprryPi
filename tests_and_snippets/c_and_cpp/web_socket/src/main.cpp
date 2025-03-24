#include "web_socket.hpp"
#include <iostream>
#include <string>

constexpr const int PORT = 31416;
constexpr const uint32_t KEEP_ALIVE = 30;

int main()
{
    WebSocketServer server;

    if (!server.start(PORT, KEEP_ALIVE))
    {
        std::cerr << "Failed to start server." << std::endl;
        return 1;
    }

    std::cout << "Enter text to send to the connected client. Type 'exit' to quit." << std::endl;
    std::string input;
    while (std::getline(std::cin, input))
    {
        if (input == "exit")
            break;
        server.send_to_client(input);
    }

    server.stop();
    return 0;
}
