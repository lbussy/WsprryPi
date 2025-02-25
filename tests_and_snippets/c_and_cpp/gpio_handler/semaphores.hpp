#ifndef __SEMAPHORES_HPP__
#define __SEMAPHORES_HPP__

#include "gpio_handler.hpp"

#include <atomic>
#include <thread>
#include <memory>

extern const int LED_PIN;
extern const int SHUTDOWN_PIN;

// Forward declare GPIOHandler
class GPIOHandler;

// Declare global variables
extern std::unique_ptr<GPIOHandler> led_handler;
extern std::unique_ptr<GPIOHandler> shutdown_handler;

extern std::thread led_thread;
extern std::thread button_thread;
extern std::atomic<bool> exit_wspr_loop;
extern std::atomic<bool> signal_shutdown;

#endif // __SEMAPHORES_HPP__
