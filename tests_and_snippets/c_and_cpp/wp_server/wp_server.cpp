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

std::atomic<int> WsprryPi_Server::active_connections = 0;

const std::unordered_set<std::string> WsprryPi_Server::valid_commands = {
    "transmit", "call", "grid", "power", "freq", "ppm", "selfcal", "offset",
    "led", "port", "xmit", "version"
};

/**
 * @brief Constructs the server instance.
 */
WsprryPi_Server::WsprryPi_Server(int port, LCBLog& logger)
    : server_fd(-1), port(port), running(false), log(logger) {}

/**
 * @brief Destructor to clean up server resources.
 */
WsprryPi_Server::~WsprryPi_Server() {
    stop();
}

/**
 * @brief Starts the server.
 */
void WsprryPi_Server::start() {
    if (running) return;
    running = true;
    server_thread = std::thread(&WsprryPi_Server::run_server, this);
}

/**
 * @brief Stops the server.
 */
void WsprryPi_Server::stop() {
    if (!running) return;
    running = false;
    
    if (server_fd != -1) {
        close(server_fd);
        server_fd = -1;
    }

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
 * @brief Main server loop.
 */
void WsprryPi_Server::run_server() {
    struct timeval accept_timeout;
    accept_timeout.tv_sec = 10;  // Wait max 10 seconds for new connections
    accept_timeout.tv_usec = 0;
    setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&accept_timeout, sizeof(accept_timeout));
    struct sockaddr_in address, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log.logE(ERROR, "Socket creation failed.");
        running = false;
        return;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log.logE(ERROR, "setsockopt failed.");
        close(server_fd);
        server_fd = -1;
        running = false;
        return;
    }

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

    if (listen(server_fd, 5) < 0) {
        log.logE(ERROR, "Listen failed.");
        close(server_fd);
        server_fd = -1;
        running = false;
        return;
    }

    log.logS(INFO, "Server listening on 127.0.0.1:" + std::to_string(port));

    while (running) {
        if (active_connections.load(std::memory_order_relaxed) >= MAX_CONNECTIONS) {
            log.logE(ERROR, "Too many connections. Rejecting client.");
            continue;
        }

        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            if (!running) break;
            log.logE(ERROR, "Accept failed.");
            continue;
        }

        if (ntohl(client_addr.sin_addr.s_addr) != INADDR_LOOPBACK) {
            log.logE(ERROR, "Rejected connection from non-localhost client.");
            close(client_socket);
            continue;
        }

        active_connections.fetch_add(1, std::memory_order_relaxed);
        std::thread(&WsprryPi_Server::handle_client, this, client_socket).detach();
    }

    close(server_fd);
    server_fd = -1;
}

void WsprryPi_Server::handle_client(int client_socket) {
    struct timeval timeout;
    timeout.tv_sec = 5;  // Close idle connections after 5 seconds
    timeout.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
    char buffer[1024] = {0};

    if (bytes_read >= sizeof(buffer) - 1) {
        log.logE(ERROR, "Received command too long. Connection closed.");
        close(client_socket);
        active_connections.fetch_sub(1, std::memory_order_relaxed);
        return;
    }

    int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        log.logE(ERROR, "Client read error or disconnected.");
        close(client_socket);
        active_connections.fetch_sub(1, std::memory_order_relaxed);
        return;
    }

    buffer[bytes_read] = '\0'; // Ensure proper string termination
    std::string input(buffer);

    // Trim leading and trailing spaces
    input.erase(0, input.find_first_not_of(" \t\r\n"));
    input.erase(input.find_last_not_of(" \t\r\n") + 1);

    std::string response = process_command(input);

    send(client_socket, response.c_str(), response.length(), 0);
    log.logS(INFO, "Processed command: " + input);

    close(client_socket);
    active_connections.fetch_sub(1, std::memory_order_relaxed);
}

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
