#include "utils.hpp"

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

// Global stop flag.
std::atomic<bool> g_stop{false};

/**
 * @brief Waits until it is exactly one second past an even minute, until spacebar is pressed, or a stop is requested.
 *
 * This function sets the terminal to noncanonical mode so that a single spacebar press is immediately
 * detected (without requiring Enter). It then periodically checks the system clock and uses select() with
 * a timeout to wait until the next second boundary. If the spacebar is pressed, the function exits early.
 *
 * @note Due to system scheduling, the wake-up time might not be exactly at the second boundary.
 */
void waitForOneSecondPastEvenMinute()
{
    // Set terminal to noncanonical mode to capture key presses immediately.
    struct termios oldAttr, newAttr;
    tcgetattr(STDIN_FILENO, &oldAttr);
    newAttr = oldAttr;
    newAttr.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newAttr);

    std::mutex m;
    std::condition_variable cv;
    std::unique_lock<std::mutex> lock(m);

    while (!g_stop.load())
    {
        // Get the current system time.
        auto now = std::chrono::system_clock::now();
        std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now = *std::localtime(&now_time_t);

        // Check if the minute is even and the seconds count is exactly 1.
        if ((tm_now.tm_min % 2 == 0) && (tm_now.tm_sec == 1))
        {
            std::cout << "Condition met: One second past an even minute." << std::endl;
            break;
        }

        // Calculate the next second boundary.
        auto next_second = now + std::chrono::seconds(1);
        auto wait_duration = std::chrono::duration_cast<std::chrono::microseconds>(next_second - now);

        // Prepare the timeout for select.
        struct timeval tv;
        tv.tv_sec = wait_duration.count() / 1000000;
        tv.tv_usec = wait_duration.count() % 1000000;

        // Set up file descriptor set for standard input.
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        // Use select to wait until either input is available or the timeout expires.
        int ret = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(STDIN_FILENO, &readfds))
        {
            // Input is available; read one character.
            char c;
            ssize_t n = read(STDIN_FILENO, &c, 1);
            if (n > 0 && c == ' ')
            {
                std::cout << "Spacebar pressed. Exiting wait early." << std::endl;
                break;
            }
        }
        // If ret == 0, the timeout expired and the loop continues.
    }

    // Restore terminal settings.
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
            ;;
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
