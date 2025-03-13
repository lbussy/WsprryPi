// signal_handler.cpp
#include "signal_handler.hpp"
#include <iostream>
#include <termios.h> // ✅ Add this header for terminal control functions
#include <unistd.h>  // ✅ Required for STDIN_FILENO

const std::unordered_map<int, std::pair<std::string_view, bool>> SignalHandler::signal_map = {
    {SIGINT, {"SIGINT", false}},
    {SIGTERM, {"SIGTERM", false}},
    {SIGQUIT, {"SIGQUIT", false}},
    {SIGHUP, {"SIGHUP", false}},
    {SIGSEGV, {"SIGSEGV", true}},
    {SIGBUS, {"SIGBUS", true}},
    {SIGFPE, {"SIGFPE", true}},
    {SIGILL, {"SIGILL", true}},
    {SIGABRT, {"SIGABRT", true}}};

SignalHandler::SignalHandler(Callback callback) : shutdown_in_progress(false), tty_saved(false), user_callback(callback)
{
    block_signals();
    suppress_terminal_signals();
    signal_thread = std::thread(&SignalHandler::signal_handler_thread, this);
}

SignalHandler::~SignalHandler()
{
    stop();
}

// Suppresses terminal signals (like disabling echo mode)
void SignalHandler::suppress_terminal_signals()
{
    struct termios tty;
    if (tcgetattr(STDIN_FILENO, &tty) == 0)
    {
        tty_saved = true;
        original_tty = tty;
        tty.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &tty);
    }
}

// Restores terminal settings after shutdown
void SignalHandler::restore_terminal_settings()
{
    if (tty_saved)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &original_tty);
    }
}

void SignalHandler::request_shutdown()
{
    shutdown_in_progress.store(true);
    cv.notify_all();
}

void SignalHandler::wait_for_shutdown()
{
    std::unique_lock<std::mutex> lock(cv_mutex);
    cv.wait(lock, [this]
            { return shutdown_in_progress.load(); });
}

void SignalHandler::set_callback(Callback callback)
{
    user_callback = callback;
}

std::string_view SignalHandler::signal_to_string(int signum)
{
    auto it = signal_map.find(signum);
    return (it != signal_map.end()) ? it->second.first : "UNKNOWN";
}

void SignalHandler::block_signals()
{
    sigset_t set;
    sigemptyset(&set);

    for (const auto &[signum, _] : signal_map)
    {
        sigaddset(&set, signum);
    }

    pthread_sigmask(SIG_BLOCK, &set, nullptr);
}

void SignalHandler::signal_handler(int signum)
{
    if (user_callback)
    {
        user_callback(signum, signal_map.at(signum).second);
    }
    else
    {
        if (signal_map.at(signum).second)
        {
            restore_terminal_settings();
            std::quick_exit(signum);
        }
        else
        {
            shutdown_in_progress.store(true);
            cv.notify_all();
        }
    }
}

void SignalHandler::signal_handler_thread()
{
    sigset_t set;
    sigemptyset(&set);

    for (const auto& [signum, _] : signal_map)
    {
        sigaddset(&set, signum);
    }

    int sig;
    while (!shutdown_in_progress.load())
    {
        int ret = sigwait(&set, &sig);
        if (ret == 0)
        {
            if (shutdown_in_progress.load()) {
                break;  // ✅ Prevents reprocessing after shutdown starts
            }

            signal_handler(sig);
        }
    }
}

void SignalHandler::stop()
{
    shutdown_in_progress.store(true);
    cv.notify_all();

    if (signal_thread.joinable())
    {
        signal_thread.join();
    }

    restore_terminal_settings();
}