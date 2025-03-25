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

extern void transmit_loop();

#endif // _TRANSMIT_HPP
