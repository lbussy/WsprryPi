#ifndef _SINGLETON_H
#define _SINGLETON_H
#pragma once

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
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-2d33a85 [new_release_proc].
*/

#include <netinet/in.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include <cstring>
#include <stdexcept>

// Usage:
//
// #include <iostream>
// #include "singleton.hpp"
//
// int main()
// {
//    SingletonProcess singleton(5555); // pick a port number to use that is specific to this app
//    if (!singleton())
//    {
//      cerr << "process running already. See " << singleton.GetLockFileName() << endl;
//      return 1;
//    }
//    // ... rest of the app
// }

class SingletonProcess
{
public:
    SingletonProcess(uint16_t port0)
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
                throw std::runtime_error(std::string("Could not create socket: ") + strerror(errno));
            }
            else
            {
                struct sockaddr_in name;
                name.sin_family = AF_INET;
                name.sin_port = htons(port);
                name.sin_addr.s_addr = htonl(INADDR_ANY);
                rc = bind(socket_fd, (struct sockaddr *)&name, sizeof(name));
            }
        }
        return (socket_fd != -1 && rc == 0);
    }

    std::string GetLockFileName()
    {
        return "port " + std::to_string(port);
    }

private:
    int socket_fd;
    int rc;
    uint16_t port;
};

#endif // _SINGLETON_H
