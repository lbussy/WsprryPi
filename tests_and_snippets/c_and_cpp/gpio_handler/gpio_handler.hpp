// Header: gpio_handler.hpp
#ifndef GPIO_HANDLER_HPP
#define GPIO_HANDLER_HPP

#include "semaphores.hpp"

#include <gpiod.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <chrono>

class GPIOHandler
{
public:
    enum class EdgeType
    {
        RISING,
        FALLING
    };
    using Callback = std::function<void(EdgeType, bool)>;

    GPIOHandler();
    GPIOHandler(int pin, bool is_input, bool pull_up,
                std::chrono::milliseconds debounce = std::chrono::milliseconds(50),
                Callback callback = nullptr);
    ~GPIOHandler();

    void setup(int pin, bool is_input, bool pull_up,
               std::chrono::milliseconds debounce = std::chrono::milliseconds(50),
               Callback callback = nullptr);

    void startMonitoring();
    void stopMonitoring();
    void setOutput(bool state);
    bool getInputState() const;

private:
    static constexpr const char *GPIO_CHIP_PATH = "/dev/gpiochip0";

    int pin_{-1};
    bool is_input_{false};
    bool pull_up_{false};
    std::chrono::milliseconds debounce_time_{50};
    Callback callback_;

    gpiod::chip chip_;
    gpiod::line line_;
    std::mutex gpio_mutex_;
    std::atomic<bool> running_{false};

    void configurePin();
    void cleanupPin();
};

#endif // GPIO_HANDLER_HPP