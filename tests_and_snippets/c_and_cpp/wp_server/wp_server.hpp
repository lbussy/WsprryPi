#ifndef WP_SERVER_HPP
#define WP_SERVER_HPP

#include <thread>
#include <string>
#include <atomic>
#include <unordered_set>
#include "lcblog.hpp"  // Include logging class

#define MAX_CONNECTIONS 10  // Adjust the limit as needed

/**
 * @class WsprryPi_Server
 * @brief A multi-threaded TCP server for handling WSPR-related commands.
 */
class WsprryPi_Server {
public:
    explicit WsprryPi_Server(int port, LCBLog& logger);  // Removed default argument

    /**
     * @brief Destructor to clean up server resources.
     */
    ~WsprryPi_Server();

    /**
     * @brief Starts the server and begins listening for connections.
     */
    void start();

    /**
     * @brief Stops the server and cleans up resources.
     */
    void stop();

private:
    static const std::unordered_set<std::string> valid_commands;
    int server_fd;   ///< File descriptor for the server socket.
    int port;        ///< Configurable port number.
    bool running;    ///< Indicates whether the server is running.
    std::thread server_thread;     ///< Thread for running the server loop.
    LCBLog& log;                   ///< Reference to logging system.
    static std::atomic<int> active_connections; ///< Active connection count.

    std::string handleTransmit(const std::string& arg);
    std::string handleCall(const std::string& arg);
    std::string handleGrid(const std::string& arg);
    std::string handlePower(const std::string& arg);
    std::string handleFreq(const std::string& arg);
    std::string handlePPM(const std::string& arg);
    std::string handleSelfCal(const std::string& arg);
    std::string handleOffset(const std::string& arg);
    std::string handleLED(const std::string& arg);
    std::string handlePort();
    std::string handleXMIT();
    std::string handleVersion();

    /**
     * @brief Main server loop.
     */
    void run_server();

    /**
     * @brief Handles an individual client connection.
     * @param client_socket The socket descriptor for the client.
     */
    void handle_client(int client_socket);

    /**
     * @brief Processes a command and generates a response.
     * @param input The command string.
     * @return The processed response string.
     */
    std::string process_command(const std::string& input);
};

#endif // WP_SERVER_HPP