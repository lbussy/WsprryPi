/**
 * @file signal_handler.cpp
 * @brief Definition file for the SignalHandler class.
 * @details Provides the SignalHandler class responsible for managing signal handling,
 *          including blocking signals, handling callbacks, and safely shutting down.
 */

 #include "signal_handler.hpp"
 #include <iostream>
 #include <termios.h>
 
 /**
  * @brief Map of signal numbers to their names and critical status.
  */
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
 
 /**
  * @brief Constructs a SignalHandler object and initializes signal handling.
  * @param callback Optional callback function for signal handling.
  */
 SignalHandler::SignalHandler(Callback callback) : shutdown_flag(false), tty_saved(false), user_callback(callback)
 {
     block_signals();
     suppress_terminal_signals();
     signal_thread = std::thread(&SignalHandler::signal_handler_thread, this);
 }
 
 /**
  * @brief Destroys the SignalHandler object and ensures cleanup.
  */
 SignalHandler::~SignalHandler()
 {
     stop();
 }
 
 /**
  * @brief Suppresses terminal signals (disables echo mode).
  */
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
 
 /**
  * @brief Restores terminal settings after shutdown.
  * @return SignalHandlerStatus indicating success or failure.
  */
 SignalHandlerStatus SignalHandler::restore_terminal_settings()
 {
     if (tty_saved)
     {
         if (tcsetattr(STDIN_FILENO, TCSANOW, &original_tty) == 0)
         {
             return SignalHandlerStatus::SUCCESS;
         }
         return SignalHandlerStatus::FAILURE;
     }
     return SignalHandlerStatus::FAILURE;
 }
 
 /**
  * @brief Requests a shutdown, ensuring it is executed only once.
  * @return SignalHandlerStatus indicating success or if already stopped.
  */
 SignalHandlerStatus SignalHandler::request_shutdown()
 {
     if (shutdown_flag.exchange(true))
     {
         return SignalHandlerStatus::ALREADY_STOPPED;
     }
     cv.notify_all();
     return SignalHandlerStatus::SUCCESS;
 }
 
 /**
  * @brief Waits for shutdown, optionally with a timeout.
  * @param timeout_seconds Optional timeout duration in seconds.
  * @return SignalHandlerStatus indicating success or timeout.
  */
 SignalHandlerStatus SignalHandler::wait_for_shutdown(std::optional<int> timeout_seconds)
 {
     std::unique_lock<std::mutex> lock(cv_mutex);
     if (timeout_seconds)
     {
         if (!cv.wait_for(lock, std::chrono::seconds(*timeout_seconds), [this] { return shutdown_flag.load(); }))
         {
             return SignalHandlerStatus::TIMEOUT;
         }
     }
     else
     {
         cv.wait(lock, [this] { return shutdown_flag.load(); });
     }
     return SignalHandlerStatus::SUCCESS;
 }
 
 /**
  * @brief Sets a callback function for signal handling.
  * @param callback Callback function to set.
  * @return SignalHandlerStatus indicating success or failure.
  */
 SignalHandlerStatus SignalHandler::set_callback(Callback callback)
 {
     if (!callback)
     {
         return SignalHandlerStatus::FAILURE;
     }
     {
         std::lock_guard<std::mutex> lock(cv_mutex);
         user_callback = std::move(callback);
     }
     return SignalHandlerStatus::SUCCESS;
 }
 
 /**
  * @brief Converts a signal number to its string representation.
  * @param signum Signal number.
  * @return String representation of the signal.
  */
 std::string_view SignalHandler::signal_to_string(int signum)
 {
     auto it = signal_map.find(signum);
     return (it != signal_map.end()) ? it->second.first : "UNKNOWN";
 }
 
 /**
  * @brief Blocks all signals listed in the signal map.
  * @return SignalHandlerStatus indicating success or failure.
  */
 SignalHandlerStatus SignalHandler::block_signals()
 {
     sigset_t set;
     sigemptyset(&set);
     bool all_added = true;
     for (const auto& [signum, _] : signal_map)
     {
         if (sigaddset(&set, signum) != 0)
         {
             all_added = false;
         }
     }
     if (!all_added)
     {
         return SignalHandlerStatus::PARTIALLY_BLOCKED;
     }
     if (pthread_sigmask(SIG_BLOCK, &set, nullptr) != 0)
     {
         return SignalHandlerStatus::FAILURE;
     }
     return SignalHandlerStatus::SUCCESS;
 }
 
 /**
  * @brief Handles a received signal.
  * @param signum Signal number received.
  */
 void SignalHandler::signal_handler(int signum)
 {
     Callback cb;
     {
         std::lock_guard<std::mutex> lock(callback_mutex);
         cb = user_callback;
     }
     if (cb)
     {
         cb(signum, signal_map.at(signum).second);
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
             request_shutdown();
         }
     }
 }
 
 /**
  * @brief Signal handling thread function.
  */
 void SignalHandler::signal_handler_thread()
 {
     sigset_t set;
     sigemptyset(&set);
     for (const auto& [signum, _] : signal_map)
     {
         sigaddset(&set, signum);
     }
     int sig;
     while (!shutdown_flag.load())
     {
         int ret = sigwait(&set, &sig);
         if (ret == 0)
         {
             if (shutdown_flag.load())
             {
                 break;
             }
             signal_handler(sig);
         }
     }
 }
 
 /**
  * @brief Stops the signal handling thread and restores terminal settings.
  * @return SignalHandlerStatus indicating success or failure.
  */
 SignalHandlerStatus SignalHandler::stop()
 {
     static std::atomic_flag stopped = ATOMIC_FLAG_INIT;
     if (stopped.test_and_set())
     {
         return SignalHandlerStatus::ALREADY_STOPPED;
     }
     cv.notify_all();
     if (signal_thread.joinable())
     {
         try
         {
             signal_thread.join();
         }
         catch (const std::system_error& e)
         {
             return SignalHandlerStatus::FAILURE;
         }
     }
     return (restore_terminal_settings() == SignalHandlerStatus::FAILURE) ?
            SignalHandlerStatus::FAILURE :
            SignalHandlerStatus::SUCCESS;
 }
 