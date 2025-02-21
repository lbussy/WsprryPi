#include "signal_handler.hpp"
#include "scheduling.hpp"
#include "arg_parser.hpp"

#include <cstring>
#include <csignal>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <termios.h>
#include <unistd.h>

extern std::atomic<bool> exit_scheduler;
extern std::condition_variable cv;
extern std::mutex cv_mtx;
std::atomic<bool> shutdown_in_progress(false);
static struct termios original_tty;  // Store original terminal settings
static bool tty_saved = false;

void suppress_terminal_signals()
{
    if (tcgetattr(STDIN_FILENO, &original_tty) == 0)
    {
        tty_saved = true;
        struct termios tty = original_tty;
        tty.c_lflag &= ~(ECHO);  // Suppress echo, keep ISIG enabled
        tcsetattr(STDIN_FILENO, TCSANOW, &tty);
    }
}

void restore_terminal_signals()
{
    if (tty_saved)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &original_tty);
    }
}

void block_signals()
{
    sigset_t set;
    sigemptyset(&set);

    // Add signals to block
    const int signals[] = {SIGINT, SIGTERM, SIGQUIT, SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGHUP, SIGABRT};
    for (int signum : signals)
    {
        sigaddset(&set, signum);
    }

    // Block signals for the current thread (main)
    pthread_sigmask(SIG_BLOCK, &set, nullptr);
    llog.logS(DEBUG, "Blocked signals in the main thread.");
}

std::string signal_to_string(int signum)
{
    static const std::unordered_map<int, std::string> signal_map = {
        {SIGINT, "SIGINT"},   {SIGTERM, "SIGTERM"}, {SIGQUIT, "SIGQUIT"},
        {SIGSEGV, "SIGSEGV"}, {SIGBUS, "SIGBUS"},   {SIGFPE, "SIGFPE"},
        {SIGILL, "SIGILL"},   {SIGHUP, "SIGHUP"},   {SIGABRT, "SIGABRT"}
    };

    auto it = signal_map.find(signum);
    return (it != signal_map.end()) ? it->second : "UNKNOWN_SIGNAL";
}

void cleanup_threads()
{
    llog.logS(DEBUG, "Cleaning up active threads.");
    exit_scheduler.store(true);
    cv.notify_all();

    for (auto &thread : active_threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    restore_terminal_signals();
    restore_governor();
    llog.logS(INFO, "Exiting Wsprry Pi");
    std::exit(0); // Ensure clean exit
}

void signal_handler(int signum)
{
    // Log the received signal
    std::string signal_name = signal_to_string(signum);
    std::ostringstream oss;
    oss << "Caught " << signal_name << ". Shutting down.";
    std::string log_message = oss.str();  // Convert to a standard string

    // Perform cleanup based on the signal type
    switch (signum)
    {
    // Fatal errors not allowing cleanup
    case SIGSEGV:
    case SIGBUS:
    case SIGFPE:
    case SIGILL:
    case SIGABRT:
        llog.logE(FATAL, log_message);
        restore_governor();
        restore_terminal_signals();
        std::quick_exit(signum); // Ensure immediate exit
        break;

    // Signals allowing cleanup
    case SIGINT:
    case SIGTERM:
    case SIGQUIT:
    case SIGHUP:
        llog.logS(INFO, log_message);
        cleanup_threads(); // Graceful cleanup
        break;

    default:
        llog.logE(FATAL, "Unknown signal caught. Exiting.");
        break;
    }

    // Ignore further signals during shutdown
    if (shutdown_in_progress.load())
    {
        llog.logE(WARN, "Shutdown already in progress. Ignoring signal:", signal);
        return;
    }

    exit_scheduler.store(true); // Ensure shutdown flag is set
    cv.notify_all();            // Wake up threads blocked on condition variable

    cleanup_threads();
}

void register_signal_handlers()
{
    block_signals();
    suppress_terminal_signals();
    std::thread([]
                {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGINT);
        sigaddset(&set, SIGTERM);
        sigaddset(&set, SIGQUIT);
        sigaddset(&set, SIGSEGV);
        sigaddset(&set, SIGBUS);
        sigaddset(&set, SIGFPE);
        sigaddset(&set, SIGILL);
        sigaddset(&set, SIGHUP);

        int sig;
        while (!shutdown_in_progress)
        {
            int ret = sigwait(&set, &sig);
            if (ret == 0)
            {
                signal_handler(sig);  // Handle the signal
            }
            else
            {
                llog.logE(ERROR, "Failed to wait for signals:", std::strerror(errno));
            }
        } })
        .detach();

    llog.logS(DEBUG, "Signal handling thread started.");
}
