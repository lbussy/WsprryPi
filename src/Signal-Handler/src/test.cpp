/**
 * @file test.cpp
 * @brief Bounded, unprivileged SignalHandler lifecycle test.
 *
 * Licensed under the repository-root LICENSE.md.
 * Copyright © 2025 - 2026 Lee C. Bussy (@LBussy). All rights reserved.
 */

#include "signal_handler.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>
#include <unistd.h>

int main()
{
    block_signals();

    std::atomic<bool> callbackCalled(false);
    std::atomic<int> callbackSignal(0);
    std::atomic<bool> callbackCritical(true);

    SignalHandler handler;
    handler.setCallback([&](int signum, bool critical) {
        callbackSignal.store(signum);
        callbackCritical.store(critical);
        callbackCalled.store(true);
    });
    handler.start();

    // This controlled signal targets only the current test process. The handled
    // set was blocked before the worker started, so the dedicated waiter owns it.
    if (::kill(::getpid(), SIGTERM) != 0)
    {
        std::cerr << "Unable to send the controlled self-test signal.\n";
        handler.stop();
        return 1;
    }

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::seconds(2);
    while (!callbackCalled.load() &&
           std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    const bool stopped = handler.stop();
    const bool stoppedAgain = handler.stop();
    const bool valid = callbackCalled.load() &&
                       callbackSignal.load() == SIGTERM &&
                       !callbackCritical.load() && stopped && !stoppedAgain &&
                       SignalHandler::signalToString(SIGTERM) == "SIGTERM" &&
                       SignalHandler::signalToString(-1) == "UNKNOWN";
    if (!valid)
    {
        std::cerr << "SignalHandler callback or shutdown contract failed.\n";
        return 1;
    }
    return 0;
}
