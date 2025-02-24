// TODO:  Check Doxygen

#ifndef _TRANSMIT_HPP
#define _TRANSMIT_HPP

#include <atomic>
#include <thread>

extern std::atomic<bool> in_transmission;
extern std::thread transmit_thread;

extern void transmit_loop();

#endif // _TRANSMIT_HPP
