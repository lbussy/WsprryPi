#include "utils.hpp"

#include <array>              // std::array for buffer in get_ppm_from_chronyc
#include <atomic>             // std::atomic for g_stop
#include <chrono>             // std::chrono used in wait_for_trans_window
#include <condition_variable> // std::condition_variable (used in earlier version, optional now)
#include <csignal>            // WIFEXITED, WIFSIGNALED, etc.
#include <cstring>            // std::strerror in get_ppm_from_chronyc()
#include <ctime>              // std::time_t, std::tm, std::localtime, std::mktime
#include <iomanip>            // std::setprecision in get_ppm_from_chronyc
#include <iostream>           // std::cout, std::cerr
#include <mutex>              // std::mutex (was used in prior versions)
#include <sstream>            // std::ostringstream, std::istringstream
#include <string>             // std::string (used widely)

#include <pthread.h>  // pthread_setschedparam
#include <sys/time.h> // struct timeval, gettimeofday, select()
#include <termios.h>  // terminal attribute handling
#include <unistd.h>   // read(), STDIN_FILENO, pipe handling, tcsetattr

// Global stop flag.
std::atomic<bool> g_stop{false};

/**
 * @brief Waits until exactly one second past an even minute, or until interrupted.
 *
 * This function calculates the exact time point corresponding to one second past the
 * next even minute and waits until that moment with microsecond-level precision. It
 * uses select() with a timeout to both wait efficiently and allow early termination
 * if the spacebar is pressed. The terminal is temporarily set to noncanonical mode
 * to allow immediate character detection without requiring Enter.
 *
 * If the current minute is odd, the function targets the next even minute. If the
 * current time is already past the target second, it waits for the subsequent
 * even minute instead.
 *
 * The function also exits early if the global stop flag is set, or if the user
 * presses the spacebar.
 *
 * @note The wait uses system_clock and may still be affected by scheduling jitter,
 * but targets the exact wake-up time without polling.
 */
void wait_for_trans_window()
{
    // Save and set terminal to noncanonical mode
    struct termios oldAttr, newAttr;
    tcgetattr(STDIN_FILENO, &oldAttr);
    newAttr = oldAttr;
    newAttr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newAttr);

    while (!g_stop.load())
    {
        using namespace std::chrono;

        auto now = system_clock::now();
        std::time_t now_tt = system_clock::to_time_t(now);
        std::tm tm_now = *std::localtime(&now_tt);

        // Compute target time: next even minute + 1 second
        if (tm_now.tm_min % 2 != 0)
            ++tm_now.tm_min; // Skip to next even minute

        tm_now.tm_sec = 1;    // One second after the even minute
        tm_now.tm_isdst = -1; // Let the system determine DST

        auto target_time = system_clock::from_time_t(std::mktime(&tm_now));
        if (target_time <= now)
            target_time += minutes(2); // Skip to the following even minute + 1 second

        // Compute sleep duration precisely
        auto wait_duration = duration_cast<microseconds>(target_time - now);

        // Use select with timeout and early exit on spacebar
        struct timeval tv;
        tv.tv_sec = wait_duration.count() / 1000000;
        tv.tv_usec = wait_duration.count() % 1000000;

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int ret = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(STDIN_FILENO, &readfds))
        {
            char c;
            ssize_t n = read(STDIN_FILENO, &c, 1);
            if (n > 0 && c == ' ')
            {
                std::cout << "Spacebar pressed. Exiting wait early." << std::endl;
                break;
            }
        }
        else
        {
            // Woke up at intended time
            std::cout << "Condition met: One second past an even minute." << std::endl;
            break;
        }
    }

    // Restore terminal
    tcsetattr(STDIN_FILENO, TCSANOW, &oldAttr);
}

/**
 * @brief Get the PPM (frequency) value from `chronyc tracking`
 *
 * Executes the `chronyc tracking` command and parses the "Frequency" line,
 * which reflects the estimated drift rate in parts per million (PPM).
 *
 * @return The frequency in PPM as a double
 * @throws std::runtime_error if the command fails or parsing fails
 */
double get_ppm_from_chronyc()
{
    const char *cmd = "chronyc tracking 2>&1";
    std::array<char, 256> buffer{};
    std::string result;

    FILE *pipe = popen(cmd, "r");
    if (!pipe)
    {
        throw std::runtime_error("Failed to run chronyc tracking.");
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    {
        result += buffer.data();
    }

    int status = pclose(pipe);

    if (status == -1)
    {
        if (errno == ECHILD)
        {
            // Treat as success
            ;
            ;
        }
        else
        {
            std::ostringstream err;
            err << "pclose() failed while closing pipe to chronyc.\n";
            err << "errno = " << errno << " (" << std::strerror(errno) << ")\n";
            err << "Command output:\n"
                << result;
            throw std::runtime_error(err.str());
        }
    }
    else if (WIFEXITED(status))
    {
        int exit_code = WEXITSTATUS(status);
        if (exit_code != 0)
        {
            std::ostringstream err;
            err << "chronyc tracking exited with status " << exit_code << ".\n";
            err << "Command output:\n"
                << result;
            throw std::runtime_error(err.str());
        }
    }
    else if (WIFSIGNALED(status))
    {
        int signal = WTERMSIG(status);
        std::ostringstream err;
        err << "chronyc tracking terminated by signal " << signal << ".\n";
        err << "Command output:\n"
            << result;
        throw std::runtime_error(err.str());
    }

    // Parse "Frequency" line
    std::istringstream iss(result);
    std::string line;
    while (std::getline(iss, line))
    {
        if (line.find("Frequency") != std::string::npos)
        {
            std::istringstream linestream(line);
            std::string label, colon;
            double ppm;
            std::string units;

            linestream >> label >> colon >> ppm >> units;
            std::cout << "PPM set to: " << std::fixed << std::setprecision(3) << ppm << std::endl;
            return ppm;
        }
    }

    throw std::runtime_error("Frequency line not found in chronyc tracking output.");
}

/**
 * @brief Computes the difference between two time values.
 * @details Calculates `t2 - t1` and stores the result in `result`. If `t2 < t1`,
 *          the function returns `1`, otherwise, it returns `0`.
 *
 * @param[out] result Pointer to `timeval` struct to store the difference.
 * @param[in] t2 Pointer to the later `timeval` structure.
 * @param[in] t1 Pointer to the earlier `timeval` structure.
 * @return Returns `1` if the difference is negative (t2 < t1), otherwise `0`.
 */
int timeval_subtract(struct timeval *result, const struct timeval *t2, const struct timeval *t1)
{
    // Compute the time difference in microseconds
    long int diff_usec = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);

    // Compute seconds and microseconds for the result
    result->tv_sec = diff_usec / 1000000;
    result->tv_usec = diff_usec % 1000000;

    // Return 1 if t2 < t1 (negative difference), otherwise 0
    return (diff_usec < 0) ? 1 : 0;
}

/**
 * @brief Formats a timestamp from a `timeval` structure into a string.
 * @details Converts the given `timeval` structure into a formatted `YYYY-MM-DD HH:MM:SS.mmm UTC` string.
 *
 * @param[in] tv Pointer to `timeval` structure containing the timestamp.
 * @return A formatted string representation of the timestamp.
 */
std::string timeval_print(const struct timeval *tv)
{
    if (!tv)
    {
        return "Invalid timeval";
    }

    char buffer[30];
    time_t curtime = tv->tv_sec;
    struct tm *timeinfo = gmtime(&curtime);

    if (!timeinfo)
    {
        return "Invalid time";
    }

    // Format the timestamp
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %T", timeinfo);

    // Construct the final string with millisecond precision
    std::ostringstream oss;
    oss << buffer << "." << ((tv->tv_usec + 500) / 1000) << " UTC";
    return oss.str();
}

/**
 * @brief Sets the scheduling priority of the current thread.
 * @details Assigns the thread to the `SCHED_FIFO` real-time scheduling policy with
 *          the specified priority. Requires superuser privileges.
 *
 * @param[in] priority The priority level to set (higher values indicate higher priority).
 */
void setSchedPriority(int priority)
{
    // Define scheduling parameters
    struct sched_param sp;
    sp.sched_priority = priority;

    // Attempt to set thread scheduling parameters to SCHED_FIFO with the given priority
    int ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);

    // Handle potential failure
    if (ret != 0)
    {
        std::cerr << "Warning: pthread_setschedparam (increase thread priority) failed with error code: "
                  << ret << std::endl;
    }
}
