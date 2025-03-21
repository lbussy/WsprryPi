// TODO:  Check Doxygen

#ifndef _TRANSMIT_HPP
#define _TRANSMIT_HPP

#include <atomic>
#include <optional>
#include <thread>
#include <vector>
#include <mutex>

extern std::mutex transmit_mtx;
extern std::atomic<bool> in_transmission;
extern std::thread transmit_thread;

/**
 * @enum ModeType
 * @brief Specifies the mode of operation for the application.
 *
 * This enumeration defines the available modes for operation.
 * - `WSPR`: Represents the WSPR (Weak Signal Propagation Reporter) transmission mode.
 * - `TONE`: Represents a test tone generation mode.
 */
enum class ModeType
{
    WSPR, ///< WSPR transmission mode
    TONE  ///< Test tone generation mode
};

extern ModeType mode;

extern void transmit_loop();

#endif // _TRANSMIT_HPP
