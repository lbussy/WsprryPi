#include "utils.hpp"

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

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
