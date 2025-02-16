/**
 * @file singleton.hpp
 * @brief A header-only class to enforce singleton behavior.
 *
 * This file is part of WsprryPi, a project originally forked from
 * threeme3/WsprryPi (no longer active on GitHub).
 *
 * However, this new code added to the project is licensed under the
 * MIT License. See LICENSE.MIT.md for more information.
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

#ifndef SINGLETON_H
#define SINGLETON_H

#include <netinet/in.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include <stdexcept>
#include <system_error>

namespace wsprrypi {

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

} // namespace wsprrypi

#endif // SINGLETON_H
