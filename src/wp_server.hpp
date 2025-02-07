/**
 * @file wp_server.hpp
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

#ifndef WP_SERVER_HPP
#define WP_SERVER_HPP

#include <thread>
#include <string>
#include <atomic>
#include <unordered_set>
#include "lcblog.hpp"  // Include logging class

#define MAX_CONNECTIONS 10  ///< Maximum number of active connections allowed.

/**
 * @class WsprryPi_Server
 * @brief A multi-threaded TCP server for handling WSPR-related commands.
 * @details This class implements a TCP server that listens on a configurable port,
 *          accepts client connections, and processes commands related to WSPR
 *          (Weak Signal Propagation Reporter). Commands can be read-only or
 *          set operations, depending on whether an argument is provided.
 */
class WsprryPi_Server {
public:
    /**
     * @brief Constructs a new WsprryPi_Server instance.
     * @details Initializes the TCP server with the specified port and a reference
     *          to an external logging system.
     *
     * @param port The port number on which the server will listen for connections.
     * @param logger Reference to an `LCBLog` instance for logging events.
     *
     * @note The server does not start automatically upon construction; `start()` must be called.
     */
    explicit WsprryPi_Server(int port, LCBLog& logger);

    /**
     * @brief Destructor to clean up server resources.
     * @details Ensures that the server stops and releases any allocated resources
     *          before the instance is destroyed.
     */
    ~WsprryPi_Server();

    /**
     * @brief Starts the server and begins listening for connections.
     * @details If the server is not already running, this method starts a new
     *          thread to handle incoming client connections.
     *
     * @note The server runs asynchronously in a separate thread.
     * @see WsprryPi_Server::stop()
     */
    void start();

    /**
     * @brief Stops the server and cleans up resources.
     * @details Shuts down the server by closing the listening socket, forcing
     *          `accept()` to break out, and joining the server thread.
     *
     * @note Ensures all resources are released before exiting.
     * @see WsprryPi_Server::start()
     */
    void stop();

private:
    /**
     * @var valid_commands
     * @brief A set of allowed command names that the server recognizes.
     * @details Commands are case-insensitive and validated against this set before execution.
     * @note Adding a new command here requires implementing its handler function.
     */
    static const std::unordered_set<std::string> valid_commands;

    int server_fd;   ///< File descriptor for the server socket.
    int port;        ///< Configurable port number.
    bool running;    ///< Indicates whether the server is currently running.
    std::thread server_thread;     ///< Thread responsible for running the server loop.
    LCBLog& log;     ///< Reference to the logging system.

    /**
     * @var active_connections
     * @brief Tracks the number of active client connections.
     * @details This atomic variable ensures thread-safe counting of active connections,
     *          preventing race conditions when multiple clients connect and disconnect.
     */
    static std::atomic<int> active_connections;

    /**
     * @brief Handles the "Transmit" command.
     * @param arg The optional argument specifying the new transmit value.
     * @return Response string with either the current transmit value or confirmation of the update.
     */
    std::string handleTransmit(const std::string& arg);

    /**
     * @brief Handles the "Call" command.
     * @param arg The optional argument specifying the new call sign.
     * @return Response string with either the current call sign or confirmation of the update.
     */
    std::string handleCall(const std::string& arg);

    /**
     * @brief Handles the "Grid" command.
     * @param arg The optional argument specifying the new grid locator.
     * @return Response string with either the current grid value or confirmation of the update.
     */
    std::string handleGrid(const std::string& arg);

    /**
     * @brief Handles the "Power" command.
     * @param arg The optional argument specifying the new power level.
     * @return Response string with either the current power level or confirmation of the update.
     */
    std::string handlePower(const std::string& arg);

    /**
     * @brief Handles the "Frequency" command.
     * @param arg The optional argument specifying the new frequency.
     * @return Response string with either the current frequency or confirmation of the update.
     */
    std::string handleFreq(const std::string& arg);

    /**
     * @brief Handles the "PPM" (Parts Per Million) command.
     * @param arg The optional argument specifying the new PPM correction.
     * @return Response string with either the current PPM value or confirmation of the update.
     */
    std::string handlePPM(const std::string& arg);

    /**
     * @brief Handles the "SelfCal" (Self Calibration) command.
     * @param arg The optional argument specifying the new self-calibration state.
     * @return Response string with either the current state or confirmation of the update.
     */
    std::string handleSelfCal(const std::string& arg);

    /**
     * @brief Handles the "Offset" command.
     * @param arg The optional argument specifying the new frequency offset.
     * @return Response string with either the current offset or confirmation of the update.
     */
    std::string handleOffset(const std::string& arg);

    /**
     * @brief Handles the "LED" command.
     * @param arg The optional argument specifying the new LED state.
     * @return Response string with either the current LED state or confirmation of the update.
     */
    std::string handleLED(const std::string& arg);

    /**
     * @brief Handles the "Port" command (read-only).
     * @return The current server port number.
     */
    std::string handlePort();

    /**
     * @brief Handles the "XMIT" (Transmit) command (read-only).
     * @return The current transmission state.
     */
    std::string handleXMIT();

    /**
     * @brief Handles the "Version" command (read-only).
     * @return The current software version of the server.
     */
    std::string handleVersion();

    /**
     * @brief Main server loop.
     * @details Accepts incoming client connections, enforcing timeouts and
     *          connection limits. Runs in a separate thread after calling `start()`.
     *
     * @note Only allows connections from `127.0.0.1` (localhost).
     * @note If `MAX_CONNECTIONS` is exceeded, new connections are rejected.
     *
     * @throws Logs an error if socket creation, binding, or listening fails.
     */
    void run_server();

    /**
     * @brief Handles an individual client connection.
     * @details Reads input from the client, processes the command, sends back
     *          the response, and closes the connection.
     *
     * @param client_socket The socket descriptor for the client.
     *
     * @note Clients that remain idle for more than 5 seconds are automatically disconnected.
     * @note If a command exceeds the buffer limit, the connection is closed immediately.
     *
     * @throws Logs errors if the client connection fails or receives an invalid command.
     */
    void handle_client(int client_socket);

    /**
     * @brief Processes a command and generates a response.
     * @details Validates the received command, ensures case-insensitivity,
     *          and forwards it to the appropriate handler function.
     *
     * @param input The command string received from the client.
     * @return The processed response string.
     *
     * @note Commands are case-insensitive and trimmed of whitespace.
     * @note Returns an error message if the command is unknown or used incorrectly.
     *
     * @throws Logs an error if an invalid command is received.
     */
    std::string process_command(const std::string& input);
};

#endif // WP_SERVER_HPP