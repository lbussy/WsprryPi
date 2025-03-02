// TODO:  Check Doxygen

// Primary header for this source file
#include "ppm_ntp.hpp"

// Project headers
#include "arg_parser.hpp"
#include "scheduling.hpp"
#include "transmit.hpp"
#include "logging.hpp"
#include "signal_handler.hpp"

// Standard library headers
#include <array>
#include <atomic>
#include <cstring>
#include <string>
#include <thread>
#include <condition_variable>

// System headers
#include <sys/timex.h>

std::mutex ppm_mtx;

std::atomic<bool> stop_ppm_ntp_monitor{false};
std::thread ppm_ntp_thread;
double last_ppm = 0.0;  ///< Stores the previous PPM value.

std::string run_command(const std::string &command)
{
    std::array<char, 128> buffer{};
    std::string result;
    FILE *pipe = popen(command.c_str(), "r");

    if (!pipe)
    {
        llog.logE(ERROR, "Failed to run command:", command);
        return "";
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    {
        result += buffer.data();
    }

    int status = pclose(pipe);
    if (status == -1)
    {
        llog.logE(ERROR, "Failed to close pipe for command:", command);
        return "";
    }

    return result.empty() ? "" : result;
}

bool is_ntp_synchronized()
{
    try
    {
        std::string result = run_command("ntpq -c 'rv 0'");

        if (result.find("sync_ntp") != std::string::npos)
        {
            llog.logS(INFO, "NTP is synchronized.");
            return true;
        }

        llog.logE(WARN, "NTP is NOT synchronized.");
        return false;
    }
    catch (const std::exception &e)
    {
        llog.logE(FATAL, "Failed to check NTP synchronization:", e.what());
        return false;
    }
}

bool check_ntp_status()
{
    std::string output = run_command("ntpq -pn");

    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line))
    {
        if (line.empty() || line.find("remote") != std::string::npos || line.find("=") != std::string::npos)
            continue;

        std::istringstream fields(line);
        std::vector<std::string> tokens;
        std::string token;

        while (fields >> token)
            tokens.push_back(token);

        if (!tokens.empty() && (tokens[0][0] == '*' || tokens[0][0] == '#'))
        {
            std::string sync_flag = tokens[0].substr(0, 1); // * or #
            std::string remote = tokens[0].substr(1);       // IP address

            tokens[0] = sync_flag;
            tokens.insert(tokens.begin() + 1, remote);

            if (tokens.size() >= 11)
            {
                std::string type = tokens[4];
                std::string when = tokens[5];
                std::string poll = tokens[6];
                std::string reach = tokens[7];
                std::string delay = tokens[8];
                std::string offset = tokens[9];
                std::string jitter = tokens[10];

                llog.logS(DEBUG, "NTP synchronized server:", remote, "\nNTP Offset:", offset,
                          "ms \nNTP Jitter:", jitter, "ms \nNTP Reach:", reach);
                llog.logS(DEBUG, "NTP synchronization verified.");
                return true;
            }
            else
            {
                llog.logE(WARN, "NTP insufficient fields in line:", line);
            }
        }
    }

    llog.logE(FATAL, "No synchronized NTP server found.");
    return false;
}

bool restart_ntp_service()
{
    llog.logS(WARN, "Restarting NTPsec service.");

    if (system("sudo systemctl restart ntpsec") != 0)
    {
        llog.logE(ERROR, "Failed to restart NTPsec service.");
        return false;
    }

    llog.logS(INFO, "NTPsec service restarted. Verifying synchronization.");
    std::this_thread::sleep_for(std::chrono::seconds(5));

    for (int i = 0; i < 5; ++i)
    {
        if (check_ntp_status())
        {
            llog.logS(INFO, "NTP synchronization restored after restart.");
            return true;
        }

        llog.logS(WARN, "Waiting for NTP synchronization.");
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    llog.logE(FATAL, "Failed to restore NTP synchronization after restart.");
    return false;
}

bool ensure_ntp_stable()
{
    constexpr int max_retries = 3;
    constexpr int initial_retry_interval = 10;
    int current_retry_interval = initial_retry_interval;

    for (int attempt = 1; attempt <= max_retries; ++attempt)
    {
        if (check_ntp_status())
        {
            return true;
        }

        llog.logE(WARN, "NTP check failed (attempt", attempt, "of", max_retries, ").");

        if (attempt < max_retries)
        {
            llog.logS(INFO, "Retrying NTP check in", current_retry_interval, "seconds.");
            std::this_thread::sleep_for(std::chrono::seconds(current_retry_interval));
            current_retry_interval *= 2;
        }
        else
        {
            llog.logE(FATAL, "NTP check failed after", max_retries, "attempts. Exiting.");
            std::exit(EXIT_FAILURE);
        }
    }

    return false;
}

bool update_ppm()
{
    struct timex ntx{};
    errno = 0;

    int status = ntp_adjtime(&ntx);
    if (status == -1)
    {
        llog.logE(FATAL, "Failed to adjust NTP time for PPM check:", std::string(strerror(errno)));
        return false;
    }

    double ppm_new = static_cast<double>(ntx.freq) / (1 << 16);
    if (std::abs(last_ppm - ppm_new) > std::numeric_limits<double>::epsilon())
    {
        llog.logS(INFO, "PPM check new value obtained:", std::fixed, std::setprecision(10), ppm_new);
        last_ppm = ppm_new;
    }
    else
    {
        llog.logS(DEBUG, "PPM value unchanged at:", std::fixed, std::setprecision(10), ppm_new);
    }

    return true;
}

void ppm_ntp_monitor_thread()
{
    while (!exit_wspr_loop.load() && !signal_shutdown.load())
    {
        auto now = std::chrono::system_clock::now();
        auto next_check = std::chrono::time_point_cast<std::chrono::seconds>(now) +
                          std::chrono::minutes(1);
        next_check -= std::chrono::seconds(next_check.time_since_epoch().count() % 60);
        next_check += std::chrono::seconds(55);

        {
            std::unique_lock<std::mutex> lock(ppm_mtx);
            if (cv.wait_until(lock, next_check, [] { return exit_wspr_loop.load() || signal_shutdown.load(); }))
            {
                break;  // Ensure immediate exit
            }
        }
    }

    llog.logS(INFO, "PPM/NTP monitor thread exited.");
}
