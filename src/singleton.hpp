#ifndef _SINGLETON_H
#define _SINGLETON_H

// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

/*
 * WsprryPi
 * Updated and maintained by Lee C. Bussy
 *
 * Originally forked from threeme3/WsprryPi (no longer active), this
 * project has been significantly updated, improved, and documented for
 * ease of use.
 *
 * Inspired by a conversation with Bruce Raymond of TAPR, this fork has
 * diverged substantially from its origins and operates as an independent
 * project.
 *
 * Contributors:
 *   - threeme3 (Original Author)
 *   - Bruce Raymond (Inspiration and Guidance)
 *   - Lee Bussy, aa0nt@arrl.net
 *
 * Copyright (C) 2023-2024 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-55ad7f3 [fix_57].
 */

#include <netinet/in.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include <cstring>
#include <stdexcept>

namespace wspr {

class SingletonProcess
{
public:
    explicit SingletonProcess(uint16_t port0)
        : socket_fd(-1), rc(1), port(port0)
    {
    }

    ~SingletonProcess()
    {
        if (socket_fd != -1)
        {
            close(socket_fd);
        }
    }

    bool operator()()
    {
        if (socket_fd == -1 || rc)
        {
            socket_fd = -1;
            rc = 1;

            if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            {
                throw std::runtime_error(
                    "Could not create socket on port " + std::to_string(port) + ": " + strerror(errno));
            }
            else
            {
                struct sockaddr_in name {};
                name.sin_family = AF_INET;
                name.sin_port = htons(port);
                name.sin_addr.s_addr = htonl(INADDR_ANY);
                rc = bind(socket_fd, reinterpret_cast<struct sockaddr *>(&name), sizeof(name));
            }
        }
        return (socket_fd != -1 && rc == 0);
    }

    [[nodiscard]] inline std::string GetLockFileName() const
    {
        return "port " + std::to_string(port);
    }

private:
    int socket_fd;
    int rc;
    uint16_t port;
};

} // namespace wspr

#endif // _SINGLETON_H
