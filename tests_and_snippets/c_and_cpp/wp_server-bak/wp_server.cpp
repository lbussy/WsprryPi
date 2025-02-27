/**
 * @file wp_server.cpp
 * @brief TCP server to get/set WsprryPi parameters.
 *
 * This file is part of WsprryPi, a project originally forked from
 * threeme3/WsprryPi (no longer active on GitHub).
 *
 * However, this new code added to the project is distributed under under
 * the MIT License. See LICENSE.MIT.md for more information.
 *
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
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

// 🚨 External Exposure Risks
// If you bind to 0.0.0.0, use a firewall (ufw) to restrict access:
// Recommended UFW: sudo ufw allow from 192.168.1.0/24 to any port 31415

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
    "led", "port", "xmit", "version"
};

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
WsprryPi_Server::WsprryPi_Server(int port, LCBLog& logger)
    : server_fd(-1), port(port), running(false), log(logger) {}

/**
 * @brief Destructor for WsprryPi_Server.
 * @details Ensures that the server stops and releases any allocated resources
 *          before the instance is destroyed. Calls `stop()` to properly shut
 *          down the server, close the socket, and terminate the server thread.
 *
 * @note If the server is still running when the destructor is called, `stop()`
 *       will be executed to perform a clean shutdown.
 */
WsprryPi_Server::~WsprryPi_Server() {
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
void WsprryPi_Server::start() {
    if (running) return;  ///< Prevent multiple starts
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
void WsprryPi_Server::stop() {
    if (!running) return;  ///< Prevent stopping an already stopped server
    running = false;

    // Close the server socket if it is open
    if (server_fd != -1) {
        close(server_fd);
        server_fd = -1;
    }

    // Ensure the server thread is safely joined
    if (server_thread.joinable()) {
        // Force break out of accept() by connecting to the server
        int tmp_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (tmp_socket >= 0) {
            struct sockaddr_in tmp_addr;
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
void WsprryPi_Server::run_server() {
    struct timeval accept_timeout;
    accept_timeout.tv_sec = 10;  // Wait max 10 seconds for new connections
    accept_timeout.tv_usec = 0;
    setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&accept_timeout, sizeof(accept_timeout));

    struct sockaddr_in address, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int opt = 1;

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log.logE(ERROR, "Socket creation failed.");
        running = false;
        return;
    }

    // Allow address reuse
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log.logE(ERROR, "setsockopt failed.");
        close(server_fd);
        server_fd = -1;
        running = false;
        return;
    }

    // Bind to localhost (127.0.0.1) and the specified port
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        log.logE(ERROR, "Bind failed.");
        close(server_fd);
        server_fd = -1;
        running = false;
        return;
    }

    // Start listening for connections
    if (listen(server_fd, 5) < 0) {
        log.logE(ERROR, "Listen failed.");
        close(server_fd);
        server_fd = -1;
        running = false;
        return;
    }

    log.logS(INFO, "Server listening on 127.0.0.1:" + std::to_string(port));

    while (running) {
        // Reject new clients if too many active connections
        if (active_connections.load(std::memory_order_relaxed) >= MAX_CONNECTIONS) {
            log.logE(ERROR, "Too many connections. Rejecting client.");
            continue;
        }

        // Accept an incoming client connection
        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            if (!running) break; // Exit loop if the server is shutting down
            log.logE(ERROR, "Accept failed.");
            continue;
        }

        // Reject non-localhost connections (security measure)
        if (ntohl(client_addr.sin_addr.s_addr) != INADDR_LOOPBACK) {
            log.logE(ERROR, "Rejected connection from non-localhost client.");
            close(client_socket);
            continue;
        }

        // Increment active connection count and start handling the client in a new thread
        active_connections.fetch_add(1, std::memory_order_relaxed);
        std::thread(&WsprryPi_Server::handle_client, this, client_socket).detach();
    }

    // Cleanup: Close the server socket before exiting
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
void WsprryPi_Server::handle_client(int client_socket) {
    // Apply a timeout to prevent idle clients from blocking the server
    struct timeval timeout;
    timeout.tv_sec = 5;  // Close idle connections after 5 seconds
    timeout.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));

    char buffer[1024] = {0};
    int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);

    // Reject commands that are too long
    if (bytes_read >= static_cast<int>(sizeof(buffer) - 1)) {
        log.logE(ERROR, "Received command too long. Connection closed.");
        close(client_socket);
        active_connections.fetch_sub(1, std::memory_order_relaxed);
        return;
    }

    // Handle client read errors or disconnections
    if (bytes_read <= 0) {
        log.logE(ERROR, "Client read error or disconnected.");
        close(client_socket);
        active_connections.fetch_sub(1, std::memory_order_relaxed);
        return;
    }

    buffer[bytes_read] = '\0'; // Ensure proper string termination
    std::string input(buffer);

    // Trim leading and trailing whitespace
    input.erase(0, input.find_first_not_of(" \t\r\n"));
    input.erase(input.find_last_not_of(" \t\r\n") + 1);

    // Process the command and generate a response
    std::string response = process_command(input);

    // Send response back to the client
    send(client_socket, response.c_str(), response.length(), 0);
    log.logS(INFO, "Processed command: " + input);

    // Close connection and decrement active connection count
    close(client_socket);
    active_connections.fetch_sub(1, std::memory_order_relaxed);
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
std::string WsprryPi_Server::process_command(const std::string& input) {
    std::string command = input;

    // Debug log to check raw input
    log.logS(INFO, "Raw received command: [" + input + "]");

    // Remove leading/trailing spaces
    command.erase(0, command.find_first_not_of(" \t\r\n"));
    command.erase(command.find_last_not_of(" \t\r\n") + 1);

    // Convert to lowercase
    std::transform(command.begin(), command.end(), command.begin(), ::tolower);

    // Debug log to check processed input
    log.logS(INFO, "Processed command: [" + command + "]");

    // Split command and argument
    auto pos = command.find(' ');
    std::string cmd = (pos == std::string::npos) ? command : command.substr(0, pos);
    std::string arg = (pos == std::string::npos) ? "" : command.substr(pos + 1);

    // Debug log to verify extracted parts
    log.logS(INFO, "Command: [" + cmd + "], Argument: [" + arg + "]");

    // Check if command exists in valid_commands
    if (valid_commands.find(cmd) == valid_commands.end()) {
        log.logE(ERROR, "Invalid command received: [" + cmd + "]");
        return "ERROR: Unknown or invalid command\n";
    }

    // Process Commands (Read if no arg, Write if arg)
    if (cmd == "transmit") return handleTransmit(arg);
    if (cmd == "call") return handleCall(arg);
    if (cmd == "grid") return handleGrid(arg);
    if (cmd == "power") return handlePower(arg);
    if (cmd == "freq") return handleFreq(arg);
    if (cmd == "ppm") return handlePPM(arg);
    if (cmd == "selfcal") return handleSelfCal(arg);
    if (cmd == "offset") return handleOffset(arg);
    if (cmd == "led") return handleLED(arg);
    if (cmd == "port" && arg.empty()) return handlePort();
    if (cmd == "xmit" && arg.empty()) return handleXMIT();

    log.logE(ERROR, "Valid command but incorrect usage: [" + cmd + "]");
    return "ERROR: Unknown or invalid command\n";
}

std::string WsprryPi_Server::handleTransmit(const std::string& arg) {
    if (arg.empty()) {
        log.logS(INFO, "Reading Transmit value");
        return "Transmit value: <dummy>\n";  // Replace <dummy> with actual read logic
    } else {
        log.logS(INFO, "Setting Transmit to: " + arg);
        return "Transmit set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handleCall(const std::string& arg) {
    if (arg.empty()) {
        log.logS(INFO, "Reading Call value");
        return "Call value: <dummy>\n";
    } else {
        log.logS(INFO, "Setting Call to: " + arg);
        return "Call set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handleGrid(const std::string& arg) {
    if (arg.empty()) {
        log.logS(INFO, "Reading Grid value");
        return "Grid value: <dummy>\n";
    } else {
        log.logS(INFO, "Setting Grid to: " + arg);
        return "Grid set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handlePower(const std::string& arg) {
    if (arg.empty()) {
        log.logS(INFO, "Reading Power value");
        return "Power value: <dummy>\n";
    } else {
        log.logS(INFO, "Setting Power to: " + arg);
        return "Power set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handleFreq(const std::string& arg) {
    if (arg.empty()) {
        log.logS(INFO, "Reading Frequency value");
        return "Frequency value: <dummy>\n";
    } else {
        log.logS(INFO, "Setting Frequency to: " + arg);
        return "Frequency set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handlePPM(const std::string& arg) {
    if (arg.empty()) {
        log.logS(INFO, "Reading PPM value");
        return "PPM value: <dummy>\n";
    } else {
        log.logS(INFO, "Setting PPM to: " + arg);
        return "PPM set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handleSelfCal(const std::string& arg) {
    if (arg.empty()) {
        log.logS(INFO, "Reading SelfCal value");
        return "SelfCal value: <dummy>\n";
    } else {
        log.logS(INFO, "Setting SelfCal to: " + arg);
        return "SelfCal set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handleOffset(const std::string& arg) {
    if (arg.empty()) {
        log.logS(INFO, "Reading Offset value");
        return "Offset value: <dummy>\n";
    } else {
        log.logS(INFO, "Setting Offset to: " + arg);
        return "Offset set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handleLED(const std::string& arg) {
    if (arg.empty()) {
        log.logS(INFO, "Reading LED value");
        return "LED value: <dummy>\n";
    } else {
        log.logS(INFO, "Setting LED to: " + arg);
        return "LED set to: " + arg + "\n";
    }
}

std::string WsprryPi_Server::handlePort() {
    log.logS(INFO, "Reading Port value");
    return "Port value: <dummy>\n";
}

std::string WsprryPi_Server::handleXMIT() {
    log.logS(INFO, "Reading XMIT value");
    return "XMIT value: <dummy>\n";
}

std::string WsprryPi_Server::handleVersion() {
    log.logS(INFO, "Reading Version value");
    return "Version value: <dummy>\n";
}
