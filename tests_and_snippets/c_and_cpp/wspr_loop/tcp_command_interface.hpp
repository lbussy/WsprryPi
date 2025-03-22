/**
 * @file tcp_command_interface.hpp
 * @brief Abstract base class for handling TCP server commands.
 * @details This file defines the interface for implementing command handlers
 *          in the TCP-Server library. Derived classes must implement methods
 *          to process and handle incoming TCP commands.
 *
 * @note This file is an override of the functional example in the TCP-Server
 *       library.
 * @see https://github.com/lbussy/TCP-Server
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

#ifndef TCP_COMMAND_INTERFACE_H
#define TCP_COMMAND_INTERFACE_H

// Standard includes
#include <optional>
#include <string>
#include <unordered_set>

/**
 * @class TCP_CommandHandler
 * @brief Abstract base class for handling TCP server commands.
 * @details This class defines the interface for command processing.
 *          Implementations must provide definitions for handling,
 *          processing, and retrieving valid commands.
 */
class TCP_CommandHandler
{
public:
    /**
     * @brief Handles a command received from the client.
     * @details This function is responsible for executing the appropriate
     *          response to a given command and its optional argument.
     *          Derived classes must implement this function.
     *
     * @param command The command string received from the client.
     * @param arg The argument associated with the command, if any.
     * @return A response string to be sent back to the client.
     */
    virtual std::string handleCommand(const std::string &command, const std::optional<std::string> &arg) = 0;

    /**
     * @brief Processes a command before handling.
     * @details Allows for command preprocessing, validation, or logging
     *          before execution.
     *
     * @param command The command string received from the client.
     * @param arg The argument associated with the command, if any.
     * @return A response string based on the processed command.
     */
    virtual std::string processCommand(const std::string &command, const std::optional<std::string> &arg) = 0;

    /**
     * @brief Retrieves the list of valid commands.
     * @details Provides a set of all recognized commands for reference.
     *          Implementations must define and populate this list.
     *
     * @return A set containing valid command strings.
     */
    virtual const std::unordered_set<std::string> &getValidCommands() const = 0;

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     * @details Defined inline to eliminate the need for a separate .cpp file.
     */
    virtual ~TCP_CommandHandler() = default;
};

#endif // TCP_COMMAND_INTERFACE_H
