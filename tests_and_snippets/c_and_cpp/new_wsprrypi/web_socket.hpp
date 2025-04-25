/**
 * @file web_socket.hpp
 * @brief A simple websocket server for Wsprry Pi server support.
 *
 * This file is part of WsprryPi, a project originally branched from
 * @threeme3's WsprryPi project (no longer on GitHub). However, now the
 * original code remains only as a memory and inspiration, and this project
 * is no longer a derivative work.
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

#ifndef WEBSOCKET_HPP
#define WEBSOCKET_HPP

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

/**
 * @brief Default keep-alive interval for WebSocket ping frames (in seconds).
 *
 * This constant defines how often the server will send a ping frame to the client
 * to maintain the connection. It can be overridden by passing a custom value
 * to `WebSocketServer::start()`.
 */
extern const uint32_t SOCKET_KEEPALIVE;

/**
 * @class WebSocketServer
 * @brief A minimal standalone WebSocket server supporting one client at a time.
 *
 * This class implements a threaded WebSocket server that listens for incoming
 * connections on a specified port. It handles the WebSocket handshake, message
 * decoding, keep-alive pinging, and sending text messages to the connected client.
 *
 * Only a single client is supported at any given time. The server is designed to be
 * used in standalone environments with no external dependencies.
 */
class WebSocketServer
{
public:
    /**
     * @brief Constructs a new WebSocketServer instance.
     */
    WebSocketServer();

    /**
     * @brief Destroys the server and cleans up resources.
     */
    ~WebSocketServer();

    /**
     * @brief Starts the WebSocket server on the given port.
     *
     * The server will listen on the specified port and spawn a thread
     * to handle incoming connections. Optionally enables a periodic
     * keep-alive ping.
     *
     * @param port Port number to bind (must be between 1024 and 49151).
     * @param keep_alive_secs Keep-alive ping interval in seconds (0 disables ping).
     * @return true if the server started successfully.
     * @return false if startup failed (e.g. bind or listen error).
     */
    bool start(uint16_t port, uint32_t keep_alive_secs = 30);

    /**
     * @brief Stops the server and closes any open connections.
     */
    void stop();

    /**
     * @brief Set scheduling policy and priority for internal threads.
     *
     * This applies the given scheduling policy and priority to both
     * the server and keep-alive threads (if active and joinable).
     *
     * @param schedPolicy Scheduling policy (e.g., SCHED_FIFO, SCHED_RR).
     * @param priority Thread priority (valid for the chosen policy).
     * @return true if all applicable threads were updated successfully.
     */
    bool set_thread_priority(int schedPolicy, int priority);

    /**
     * @brief Sends a text message to the connected client.
     *
     * The message is encoded as a WebSocket text frame.
     *
     * @param message The text string to send.
     */
    void send_to_client(const std::string &message);

private:
    int listen_fd_;                         ///< Socket file descriptor for listening.
    std::atomic<bool> running_;             ///< Indicates whether the server is running.
    int client_sock_;                       ///< Socket for the active client connection.
    uint32_t keep_alive_secs_;              ///< Interval for keep-alive pings.
    std::condition_variable keep_alive_cv_; ///< Conditional to break from loop
    std::mutex keep_alive_mutex_;           ///< Mutex to control loop break

    std::thread server_thread_;     ///< Thread running the main server loop.
    std::thread keep_alive_thread_; ///< Thread for sending periodic pings.
    std::mutex client_mutex_;       ///< Mutex to guard access to client socket.

    /**
     * @brief Main server loop that accepts and manages the client connection.
     */
    void server_loop();

    /**
     * @brief Loop for periodically sending WebSocket ping frames.
     *
     * Runs in a background thread while the server is active.
     *
     * @param interval Ping interval in seconds.
     */
    void keep_alive_loop(uint32_t interval);

    /**
     * @brief Performs the WebSocket handshake with the connected client.
     *
     * Validates the client's HTTP Upgrade request and sends the server response.
     *
     * @param client Socket descriptor for the client.
     * @return true if handshake succeeded, false otherwise.
     */
    bool perform_handshake(int client);

    /**
     * @brief Trims whitespace from both ends of a string.
     *
     * Whitespace includes spaces, tabs, line feeds, and carriage returns.
     *
     * @param s The string to trim.
     * @return A new string with leading and trailing whitespace removed.
     */
    std::string trim(const std::string &s);

    /**
     * @brief Converts a string to lowercase (ASCII only).
     *
     * @param s Input string.
     * @return Lowercase version of the input string.
     */
    std::string to_lower(const std::string &s);

    /**
     * @brief Handles incoming messages from the client.
     *
     * Processes text messages and dispatches actions based on content.
     *
     * @param message The decoded and unmasked client message.
     */
    void handle_message(const std::string &message);

    /**
     * @brief Encodes binary data as a Base64 string.
     *
     * Used for computing the Sec-WebSocket-Accept header.
     *
     * @param data Pointer to binary input.
     * @param len Number of bytes to encode.
     * @return Base64-encoded string.
     */
    std::string base64_encode(const unsigned char *data, size_t len);

    /**
     * @brief Computes the WebSocket accept key from the client-provided key.
     *
     * Concatenates the GUID and applies SHA1 + Base64 encoding.
     *
     * @param client_key The value from Sec-WebSocket-Key header.
     * @return The Sec-WebSocket-Accept value to send back to the client.
     */
    std::string compute_websocket_accept(const std::string &client_key);

    /**
     * @brief Decodes a WebSocket frame from raw socket data.
     *
     * Handles payload extraction, unmasking, and opcode parsing.
     *
     * @param data Pointer to input buffer (raw socket bytes).
     * @param data_len Length of input data in bytes.
     * @param frame_size Outputs the total size of the complete frame.
     * @param opcode Outputs the opcode of the WebSocket frame (e.g. 0x1 for text).
     * @return The unmasked payload string (text data if opcode is 0x1).
     */
    std::string decode_websocket_frame(const char *data, size_t data_len, size_t &frame_size, uint8_t &opcode);
};

/**
 * @brief Global instance of the WebSocket server.
 *
 * This external declaration allows other translation units to access
 * and control the WebSocketServer instance. It is typically defined in
 * a `.cpp` file to coordinate startup, shutdown, and message routing.
 */
extern WebSocketServer socketServer;

#endif // WEBSOCKET_HPP
