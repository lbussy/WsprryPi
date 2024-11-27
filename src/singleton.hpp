/**
 * @file singleton.hpp
 * @brief Defines the SingletonProcess class for managing single-instance behavior using a socket.
 * 
 * This file is part of WsprryPi, forked from threeme3/WsprryPi (no longer
 * active).
 * 
 * Copyright (C) @threeme3 (unknown dates)
 * Copyright (C) 2023-2024 Lee C. Bussy (@LBussy). All rights reserved.
 * 
 * WsprryPi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SINGLETON_H
#define SINGLETON_H

#include <netinet/in.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include <stdexcept>
#include <system_error>

namespace wspr {

/**
 * @class SingletonException
 * @brief A custom exception for errors in the SingletonProcess class.
 */
class SingletonException : public std::runtime_error {
public:
    /**
     * @brief Constructs a SingletonException with a given message.
     * @param message The error message.
     */
    explicit SingletonException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @class SingletonProcess
 * @brief Ensures a single instance by binding to a specific port.
 */
class SingletonProcess {
public:
    /**
     * @brief Constructs a SingletonProcess.
     * @param port The port number to bind for enforcing single instance.
     */
    explicit SingletonProcess(uint16_t port)
        : socket_fd_(-1), rc_(1), port_(port) {}

    /**
     * @brief Destructor to clean up resources.
     */
    ~SingletonProcess()
    {
        if (socket_fd_ != -1) {
            close(socket_fd_);
        }
    }

    /**
     * @brief Binds to the specified port to enforce single instance behavior.
     * @return True if binding was successful, false otherwise.
     * @throws std::system_error if socket creation or binding fails.
     */
    [[nodiscard]] bool operator()()
    {
        if (socket_fd_ == -1 || rc_) {
            socket_fd_ = -1;
            rc_ = 1;

            if ((socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                throw std::system_error(errno, std::generic_category(),
                                        "Could not create socket on port " + std::to_string(port_));
            }

            struct sockaddr_in name {};
            name.sin_family = AF_INET;
            name.sin_port = htons(port_);
            name.sin_addr.s_addr = htonl(INADDR_ANY);

            rc_ = bind(socket_fd_, reinterpret_cast<struct sockaddr*>(&name), sizeof(name));
            if (rc_ != 0) {
                throw std::system_error(errno, std::generic_category(),
                                        "Could not bind to port " + std::to_string(port_));
            }
        }
        return (socket_fd_ != -1 && rc_ == 0);
    }

    /**
     * @brief Retrieves the lock's identifying name.
     * @return The lock name (currently the port as a string).
     */
    [[nodiscard]] inline std::string GetLockFileName() const
    {
        return "port " + std::to_string(port_);
    }

private:
    int socket_fd_;  ///< File descriptor for the socket.
    int rc_;         ///< Return code from the bind operation.
    uint16_t port_;  ///< Port number used for binding.
};

} // namespace wspr

#endif // SINGLETON_H
