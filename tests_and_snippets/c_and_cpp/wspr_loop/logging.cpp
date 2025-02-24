// TODO:  Check Doxygen

#include "logging.hpp"

#include "version.hpp"

/**
 * @brief Global instance of the LCBLog logging utility.
 *
 * The `llog` object provides thread-safe logging functionality with support for
 * multiple log levels, including DEBUG, INFO, WARN, ERROR, and FATAL.
 * It is used throughout the application to log messages for debugging,
 * monitoring, and error reporting.
 *
 * This instance is initialized globally to allow consistent logging across all
 * modules. Log messages can include timestamps and are output to standard streams
 * or log files depending on the configuration.
 *
 * Example usage:
 * @code
 * llog.logS(INFO, "Application started.");
 * llog.logE(ERROR, "Failed to open configuration file.");
 * @endcode
 *
 * @see https://github.com/lbussy/LCBLog for detailed documentation and examples.
 */
LCBLog llog;

/**
 * @brief Initializes the logger with the appropriate log level.
 *
 * This function sets the log level based on the current debug state. If the
 * build is compiled with the DEBUG_BUILD macro, the log level is set to DEBUG.
 * Otherwise, it defaults to INFO.
 *
 * @note Ensure that the `get_debug_state()` function correctly reflects the
 *       build configuration for accurate log level assignment.
 *
 * @example
 * initialize_logger();
 * // Sets llog to DEBUG or INFO depending on the build mode.
 */
void initialize_logger()
{
    // Determine the log level based on the build mode.
    const std::string debug_state = get_debug_state();

    // Set the appropriate log level.
    if (debug_state == "DEBUG")
    {
        llog.setLogLevel(DEBUG);  // Enable detailed debug logging.
    }
    else
    {
        llog.setLogLevel(INFO);   // Default to informational logging.
    }
}

