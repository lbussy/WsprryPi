/**
 * @file gpio_handler.hpp
 * @brief Manages GPIO pins using the libgpiod library.
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

#ifndef GPIO_HANDLER_HPP
#define GPIO_HANDLER_HPP

// Primary header for this source file
// ...

// Project headers
// ...

// Standard library headers
#include <atomic>
#include <condition_variable>
#include <functional>
#include <gpiod.hpp>
#include <mutex>
#include <thread>

// System headers
// ...

class GPIOHandler;

extern std::unique_ptr<GPIOHandler> led_handler;
extern std::unique_ptr<GPIOHandler> shutdown_handler;
extern std::thread button_thread;
extern std::thread led_thread;

extern const int LED_PIN;
extern const int SHUTDOWN_PIN;

class GPIOHandler
{
public:
    enum class EdgeType { RISING, FALLING };
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
    bool hasThreadExited() const;

private:
    static constexpr const char *GPIO_CHIP_PATH = "/dev/gpiochip0";

    int pin_{-1};
    bool is_input_{false};
    bool pull_up_{false};
    std::chrono::milliseconds debounce_time_{50};
    Callback callback_;

    gpiod::chip chip_{GPIO_CHIP_PATH};
    gpiod::line line_;

    std::atomic<bool> running_{false};
    std::atomic<bool> thread_exited_{false};

    std::mutex exit_mutex_;
    std::condition_variable exit_cv_;

    std::mutex gpio_mutex_;

    void monitoringLoop();     // Private: Runs the actual monitoring logic.
    void configurePin();
    void cleanupPin();
};

#endif // GPIO_HANDLER_HPP
