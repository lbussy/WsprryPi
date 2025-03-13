#ifndef SIGNAL_HANDLER_HPP
#define SIGNAL_HANDLER_HPP

#include <unordered_map>
#include <string_view>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <csignal>
#include <atomic>
#include <termios.h>
#include <functional>

class SignalHandler
{
public:
    using Callback = std::function<void(int, bool)>;

    explicit SignalHandler(Callback callback = {});
    ~SignalHandler();

    void request_shutdown();
    void wait_for_shutdown();
    void set_callback(Callback callback);
    void block_signals();                                 // Move this to public
    void stop();                                          // Move this to public
    static std::string_view signal_to_string(int signum); // Move this to public

private:
    static const std::unordered_map<int, std::pair<std::string_view, bool>> signal_map;
    struct termios original_tty;

    std::atomic<bool> shutdown_in_progress;
    std::atomic<bool> tty_saved;
    Callback user_callback;
    std::thread signal_thread;
    std::mutex cv_mutex;
    std::condition_variable cv;

    void signal_handler(int signum);
    void signal_handler_thread();
    void restore_terminal_settings();
    void suppress_terminal_signals();
};

#endif // SIGNAL_HANDLER_HPP