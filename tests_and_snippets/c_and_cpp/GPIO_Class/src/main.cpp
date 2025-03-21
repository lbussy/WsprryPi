#include "gpio_input.hpp"
#include "gpio_output.hpp"

#include <chrono>
#include <ctime>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <csignal>

// Global instance
GPIOInput shutdown_monitor;
GPIOOutput led_control;

// Variables to control program execution
std::mutex cv_mutex;
std::condition_variable cv;
bool stop_requested = false;

void signal_handler(int signum)
{
    std::cout << "\nCaught signal " << signum << ", stopping gracefully." << std::endl;
    {
        std::lock_guard<std::mutex> lock(cv_mutex);
        stop_requested = true;
    }
    cv.notify_all();
}

void worker_thread(int id)
{
    std::unique_lock<std::mutex> lock(cv_mutex);
    while (!stop_requested)
    {
        for (volatile int i = 0; i < 1000000; ++i)
            ;
        cv.wait_for(lock, std::chrono::milliseconds(100), []
                    { return stop_requested; });
    }
}

void callback_shutdown()
{
    std::cout << "GPIO trigger detected." << std::endl;
    for (int i = 0; i < 3; ++i)
    {
        led_control.toggle_gpio(true); // Set pin active (high)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        led_control.toggle_gpio(false); // Set pin inactive (low)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    std::lock_guard<std::mutex> lock(cv_mutex);
    stop_requested = true;
    cv.notify_all();
}

int main()
{
    std::signal(SIGINT, signal_handler);

    // Initialize GPIOInput:
    // Monitor GPIO pin 19, trigger on low, with an internal pull-up.
    if (!shutdown_monitor.enable(19, false, GPIOInput::PullMode::PullUp, callback_shutdown))
    {
        std::cerr << "Failed to initialize GPIO monitor." << std::endl;
        return 1;
    }

    // Optionally, set CPU priority (e.g., priority 50 in SCHED_FIFO range)
    if (!shutdown_monitor.setCPUPriority(SCHED_FIFO))
    {
        std::cerr << "Failed to set CPU priority." << std::endl;
    }

    // Enable output on GPIO 18 active high˝
    if (!led_control.enable_gpio_pin(18, true))
    {
        std::cerr << "Failed to enable GPIO." << std::endl;
        return 1;
    }

    std::vector<std::thread> workers;
    const int num_workers = 4;
    for (int i = 0; i < num_workers; ++i)
    {
        workers.emplace_back(worker_thread, i);
    }

    {
        std::unique_lock<std::mutex> lock(cv_mutex);
        cv.wait(lock, []
                { return stop_requested; });
    }

    // Stop the GPIO monitor before exiting.
    shutdown_monitor.stop();
    led_control.stop_gpio_pin();

    std::cout << "Waiting for worker threads to finish." << std::endl;
    for (auto &worker : workers)
    {
        worker.join();
    }

    std::cout << "All threads stopped. Exiting." << std::endl;
    return 0;
}