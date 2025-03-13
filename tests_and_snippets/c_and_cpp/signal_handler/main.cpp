#include "signal_handler.hpp"
#include <iostream>

SignalHandler *handler = nullptr;

// Custom callback function
void custom_signal_handler(int signum, bool is_critical)
{
    std::string_view signal_name = SignalHandler::signal_to_string(signum);
    std::cout << "[DEBUG] Callback executed for signal " << signal_name << std::endl;

    if (is_critical)
    {
        std::cerr << "[ERROR] Critical signal received: " << signal_name << ". Performing immediate shutdown." << std::endl;
        std::quick_exit(signum);
    }
    else
    {
        std::cout << "[INFO] Intercepted signal: " << signal_name << ". Shutdown will proceed." << std::endl;

        if (handler) // Ensure `handler` is valid
        {
            handler->request_shutdown();
        }
        else
        {
            std::cerr << "[ERROR] Handler is null. Cannot request shutdown." << std::endl;
        }
    }
}

int main()
{
    handler = new SignalHandler();

    handler->block_signals();
    handler->set_callback(custom_signal_handler);

    std::cout << "Application running. Press Ctrl+C to exit." << std::endl;
    handler->wait_for_shutdown();

    handler->stop();
    delete handler;
    handler = nullptr;

    return 0;
}