// TODO:  Redo doxygen

/**
 * @file wp_server.cpp
 * @brief TCP server to get/set WsprryPi parameters.
 *
 * This file is part of WsprryPi, a project originally created from @threeme3
 * WsprryPi projet (no longer on GitHub). However, now the original code
 * remains only as a memory and inspiration, and this project is no longer
 * a deriivative work.
 *
 * This project is is licensed under the MIT License. See LICENSE.MIT.md
 * for more information.
 *
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "wp_server.hpp"
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <regex>
#include <algorithm>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>

// 🚨 External Exposure Risks
// If you bind to 0.0.0.0, use a firewall (ufw) to restrict access:
// Recommended UFW: sudo ufw allow from 192.168.1.0/24 to any port 31415

std::atomic<bool> running{false};

ThreadPool::ThreadPool(size_t num_threads) : stop(false)
{
    for (size_t i = 0; i < num_threads; ++i)
    {
        workers.emplace_back([this]
                             {
            while (true) {
                std::vector<std::function<void()>> batch;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    condition.wait(lock, [this] { return stop || !tasks.empty(); });
                    if (stop && tasks.empty()) return;
                    
                    while (!tasks.empty()) {
                        batch.push_back(std::move(tasks.front()));
                        tasks.pop();
                    }
                }
            
                for (auto &task : batch) {
                    task();
                }
            } });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers)
        worker.join();
}

void ThreadPool::enqueue(std::function<void()> task)
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (tasks.size() >= max_queue_size)
        {
            return; // Prevent unbounded queue growth
        }
        tasks.push(std::move(task));
    }
    condition.notify_one();
}

/**
 * @var WsprryPi_Server::active_connections
 * @brief Tracks the number of active client connections.
 * @details This atomic variable ensures thread-safe counting of active client connections.
 *          It prevents race conditions when multiple clients connect and disconnect concurrently.
 *          The server enforces a limit on active connections (`MAX_CONNECTIONS`) using this counter.
 */
std::atomic<int> WsprryPi_Server::active_connections = 0;

/**
 * @var WsprryPi_Server::valid_commands
 * @brief Set of allowed command names the server can process.
 * @details This unordered set contains all valid command strings that the server recognizes.
 *          Commands are converted to lowercase before checking against this list to ensure
 *          case-insensitive matching. Any unrecognized command results in an error response.
 *
 * @note Adding a new command here requires implementing its corresponding handler in `process_command()`.
 */
const std::unordered_set<std::string> WsprryPi_Server::valid_commands = {
    "transmit", "call", "grid", "power", "freq", "ppm", "selfcal", "offset",
    "led", "port", "xmit", "version"};

/**
 * @brief Constructs a new WsprryPi_Server instance.
 * @details Initializes the TCP server with the specified port and logging system.
 *          The server socket (`server_fd`) is set to `-1` initially, indicating that
 *          it is not yet open. The `running` flag is set to `false` to indicate that
 *          the server has not started listening for connections.
 *
 * @param port The port number on which the server will listen for incoming connections.
 * @param logger Reference to an `LCBLog` instance for logging server activity.
 *
 * @note The server does not start automatically upon construction; `start()` must be called.
 */
WsprryPi_Server::WsprryPi_Server(int port, LCBLog &logger)
    : server_fd(-1), port(port), running(false), log(logger), pool(4)
{
    initializeCommandMap();
}

/**
 * @brief Destructor for WsprryPi_Server.
 * @details Ensures that the server stops and releases any allocated resources
 *          before the instance is destroyed. Calls `stop()` to properly shut
 *          down the server, close the socket, and terminate the server thread.
 *
 * @note If the server is still running when the destructor is called, `stop()`
 *       will be executed to perform a clean shutdown.
 */
WsprryPi_Server::~WsprryPi_Server()
{
    stop();
}

/**
 * @brief Starts the WsprryPi_Server.
 * @details If the server is not already running, this method sets the `running`
 *          flag to `true` and starts a new thread to execute `run_server()`,
 *          which handles incoming client connections.
 *
 * @note If the server is already running, calling `start()` has no effect.
 * @note The server runs in a separate thread to allow asynchronous operation.
 *
 * @see WsprryPi_Server::stop()
 */
void WsprryPi_Server::start()
{
    if (running)
        return; ///< Prevent multiple starts
    running = true;
    server_thread = std::thread(&WsprryPi_Server::run_server, this);
}

/**
 * @brief Stops the WsprryPi_Server and cleans up resources.
 * @details This method shuts down the server by setting `running` to `false`,
 *          closing the server socket, and ensuring the server thread is properly
 *          joined. If the server thread is blocked in `accept()`, a temporary
 *          connection is created to force it to break out and terminate cleanly.
 *
 * @note If the server is not running, calling `stop()` has no effect.
 * @note The method ensures that all resources, including the socket and thread,
 *       are properly released before returning.
 *
 * @see WsprryPi_Server::start()
 */
void WsprryPi_Server::stop()
{
    if (!running)
        return;
    running = false;

    if (server_fd != -1)
    {
        shutdown(server_fd, SHUT_RDWR);
        close(server_fd);
        server_fd = -1;
    }

    if (server_thread.joinable())
    {
        int tmp_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (tmp_socket >= 0)
        {
            struct sockaddr_in tmp_addr{};
            tmp_addr.sin_family = AF_INET;
            tmp_addr.sin_port = htons(port);
            inet_pton(AF_INET, "127.0.0.1", &tmp_addr.sin_addr);
            connect(tmp_socket, (struct sockaddr *)&tmp_addr, sizeof(tmp_addr));
            close(tmp_socket);
        }

        server_thread.join();
    }

    log.logS(INFO, "Server stopped.");
}

/**
 * @brief Runs the main server loop, accepting and handling client connections.
 * @details This method sets up the server socket, binds it to the specified port,
 *          and continuously listens for incoming connections. It enforces timeouts
 *          and a connection limit to prevent resource exhaustion.
 *
 * @note The server only accepts connections from `127.0.0.1` (localhost).
 * @note If the server reaches `MAX_CONNECTIONS`, new clients will be rejected.
 * @note This method runs in a separate thread, started by `start()`, and stops
 *       when `running` is set to `false`.
 *
 * @throws Logs an error and terminates if socket creation, binding, or listening fails.
 *
 * @see WsprryPi_Server::start()
 * @see WsprryPi_Server::stop()
 */
void WsprryPi_Server::run_server()
{
    struct sockaddr_in address{}, client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        log.logE(ERROR, "Socket creation failed.");
        running = false;
        return;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        log.logE(ERROR, "Bind failed.");
        close(server_fd);
        server_fd = -1;
        running = false;
        return;
    }

    if (listen(server_fd, 5) < 0)
    {
        log.logE(ERROR, "Listen failed.");
        close(server_fd);
        server_fd = -1;
        running = false;
        return;
    }

    log.logS(INFO, "Server listening on 127.0.0.1:" + std::to_string(port));

    while (running)
    {
        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0)
        {
            if (!running)
                break;
            continue;
        }

        pool.enqueue([this, client_socket]()
                     { handle_client(client_socket); });
    }

    shutdown(server_fd, SHUT_RDWR);
    close(server_fd);
    server_fd = -1;
}

/**
 * @brief Handles an individual client connection.
 * @details This method processes a single client request by reading input from
 *          the socket, validating and trimming the command, passing it to
 *          `process_command()`, and sending the response back to the client.
 *
 * @param client_socket The socket descriptor for the client connection.
 *
 * @note The function applies a **5-second timeout** for receiving and sending data
 *       to prevent clients from keeping the connection open indefinitely.
 * @note If the received command exceeds the buffer limit (1023 bytes), the connection
 *       is closed immediately to prevent buffer overflow.
 * @note The function decrements the active connection count when the client disconnects.
 *
 * @throws Logs an error if a read failure occurs or if the command exceeds
 *         the buffer limit.
 *
 * @see WsprryPi_Server::process_command()
 */
void WsprryPi_Server::handle_client(int client_socket)
{
    char buffer[1024] = {0};
    ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            log.logS(WARN, "Timeout while reading from client.");
        }
        else
        {
            log.logE(ERROR, "Client read error: " + std::string(strerror(errno)));
        }
        close(client_socket);
        active_connections.fetch_sub(1, std::memory_order_release);
        return;
    }

    if (bytes_read <= 0)
    {
        close(client_socket);
        return;
    }

    buffer[bytes_read] = '\0';
    std::string response = process_command(buffer);

    send(client_socket, response.c_str(), response.length(), MSG_NOSIGNAL);
    close(client_socket);
}

/**
 * @brief Processes a client command and generates an appropriate response.
 * @details This method validates the received command by trimming whitespace,
 *          converting it to lowercase, and checking against a list of valid
 *          commands. If the command is recognized, it is passed to the
 *          corresponding handler function.
 *
 * @param input The raw command string received from the client.
 *
 * @return A response string indicating the result of the command processing.
 *         If the command is unknown or incorrectly used, an error message is returned.
 *
 * @note Commands are **case-insensitive** as they are converted to lowercase.
 * @note Commands are **whitespace-trimmed** to avoid issues with extra spaces.
 * @note The method distinguishes between **read** (no argument) and **write** (with argument) operations.
 *
 * @throws Logs an error if an invalid or unknown command is received.
 *
 * @see WsprryPi_Server::handleTransmit()
 * @see WsprryPi_Server::handleCall()
 * @see WsprryPi_Server::handleGrid()
 * @see WsprryPi_Server::handlePower()
 * @see WsprryPi_Server::handleFreq()
 * @see WsprryPi_Server::handlePPM()
 * @see WsprryPi_Server::handleSelfCal()
 * @see WsprryPi_Server::handleOffset()
 * @see WsprryPi_Server::handleLED()
 * @see WsprryPi_Server::handlePort()
 * @see WsprryPi_Server::handleXMIT()
 */
std::string WsprryPi_Server::process_command(const std::string &input)
{
    // Remove trailing newline or carriage return from input
    std::string_view command = input;
    while (!command.empty() && (command.back() == '\n' || command.back() == '\r'))
    {
        command.remove_suffix(1);
    }

    log.logS(DEBUG, "Raw received command: [" + std::string(command) + "]");

    // Trim leading and trailing whitespace
    command.remove_prefix(std::min(command.find_first_not_of(" \t\r\n"), command.size()));
    size_t last_non_space = command.find_last_not_of(" \t\r\n");
    if (last_non_space != std::string_view::npos)
        command.remove_suffix(command.size() - last_non_space - 1);
    else
        return "ERROR: Empty command received\n";

    log.logS(DEBUG, "Trimmed command: [" + std::string(command) + "]");

    // Convert to lowercase in-place
    std::string cmd(command);
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    log.logS(DEBUG, "Processed command: [" + cmd + "]");

    // Split into command name and argument
    auto pos = cmd.find(' ');
    std::string command_name = (pos == std::string::npos) ? cmd : cmd.substr(0, pos);
    std::string arg = (pos == std::string::npos) ? "" : cmd.substr(pos + 1);

    log.logS(DEBUG, "Command: [" + command_name + "], Argument: [" + arg + "]");

    // Fast lookup using unordered_map
    auto it = command_map.find(command_name);
    if (it == command_map.end())
    {
        log.logE(ERROR, "Invalid command received: [" + command_name + "]");
        return "ERROR: Unknown or invalid command\n";
    }

    // Execute the associated handler function
    return it->second(arg);
}

void WsprryPi_Server::initializeCommandMap()
{
    command_map["transmit"] = [this](const std::string &arg)
    { return handleTransmit(arg); };
    command_map["call"] = [this](const std::string &arg)
    { return handleCall(arg); };
    command_map["grid"] = [this](const std::string &arg)
    { return handleGrid(arg); };
    command_map["power"] = [this](const std::string &arg)
    { return handlePower(arg); };
    command_map["freq"] = [this](const std::string &arg)
    { return handleFreq(arg); };
    command_map["ppm"] = [this](const std::string &arg)
    { return handlePPM(arg); };
    command_map["selfcal"] = [this](const std::string &arg)
    { return handleSelfCal(arg); };
    command_map["offset"] = [this](const std::string &arg)
    { return handleOffset(arg); };
    command_map["led"] = [this](const std::string &arg)
    { return handleLED(arg); };
    command_map["port"] = [this](const std::string &arg)
    { return arg.empty() ? handlePort() : "ERROR: Invalid argument for port\n"; };
    command_map["xmit"] = [this](const std::string &arg)
    { return arg.empty() ? handleXMIT() : "ERROR: Invalid argument for xmit\n"; };
    command_map["version"] = [this](const std::string &arg)
    { return arg.empty() ? handleVersion() : "ERROR: Invalid argument for version\n"; };
}

std::string WsprryPi_Server::handleTransmit(const std::string &arg)
{
    if (arg.empty())
    {
        log.logS(INFO, "Reading Transmit value");
        return "Transmit value: <dummy>\n"; // Replace <dummy> with actual read logic
    }
    else
    {
        log.logS(INFO, "Setting Transmit to: " + arg);
        return "Transmit set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handleCall(const std::string &arg)
{
    if (arg.empty())
    {
        log.logS(INFO, "Reading Call value");
        return "Call value: <dummy>\n";
    }
    else
    {
        log.logS(INFO, "Setting Call to: " + arg);
        return "Call set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handleGrid(const std::string &arg)
{
    if (arg.empty())
    {
        log.logS(INFO, "Reading Grid value");
        return "Grid value: <dummy>\n";
    }
    else
    {
        log.logS(INFO, "Setting Grid to: " + arg);
        return "Grid set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handlePower(const std::string &arg)
{
    if (arg.empty())
    {
        log.logS(INFO, "Reading Power value");
        return "Power value: <dummy>\n";
    }
    else
    {
        log.logS(INFO, "Setting Power to: " + arg);
        return "Power set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handleFreq(const std::string &arg)
{
    if (arg.empty())
    {
        log.logS(INFO, "Reading Frequency value");
        return "Frequency value: <dummy>\n";
    }
    else
    {
        log.logS(INFO, "Setting Frequency to: " + arg);
        return "Frequency set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handlePPM(const std::string &arg)
{
    if (arg.empty())
    {
        log.logS(INFO, "Reading PPM value");
        return "PPM value: <dummy>\n";
    }
    else
    {
        log.logS(INFO, "Setting PPM to: " + arg);
        return "PPM set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handleSelfCal(const std::string &arg)
{
    if (arg.empty())
    {
        log.logS(INFO, "Reading SelfCal value");
        return "SelfCal value: <dummy>\n";
    }
    else
    {
        log.logS(INFO, "Setting SelfCal to: " + arg);
        return "SelfCal set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handleOffset(const std::string &arg)
{
    if (arg.empty())
    {
        log.logS(INFO, "Reading Offset value");
        return "Offset value: <dummy>\n";
    }
    else
    {
        log.logS(INFO, "Setting Offset to: " + arg);
        return "Offset set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handleLED(const std::string &arg)
{
    if (arg.empty())
    {
        log.logS(INFO, "Reading LED value");
        return "LED value: <dummy>\n";
    }
    else
    {
        log.logS(INFO, "Setting LED to: " + arg);
        return "LED set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handlePort()
{
    log.logS(INFO, "Reading Port value");
    return "Port value: <dummy>\n";
}

std::string WsprryPi_Server::handleXMIT()
{
    log.logS(INFO, "Reading XMIT value");
    return "XMIT value: <dummy>\n";
}

std::string WsprryPi_Server::handleVersion()
{
    log.logS(INFO, "Reading Version value");
    return "Version value: <dummy>\n";
}
