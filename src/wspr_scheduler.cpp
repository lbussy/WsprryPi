/**
 * @file wspr_scheduler.hpp
 * @brief A C++ class designed to handle the timing of WSPR Transmissions.
 *
 * This file is part of WsprryPi, a project originally branched from
 * @threeme3's WsprryPi project (no longer on GitHub). However, now the
 * original code remains only as a memory and inspiration, and this project
 * is no longer a derivative work.
 *
 * This project is is licensed under the MIT License. See LICENSE.MIT.md
 * for more information.
 *
 * Copyright (C) 2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
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

#include "wspr_scheduler.hpp"

#include "web_socket.hpp"
#include "lcblog.hpp"
#include "json.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>

WSPR_Scheduler wspr_scheduler; // Global instance

WSPR_Scheduler::WSPR_Scheduler()
    : thread_policy_(SCHED_OTHER), // default policy
      thread_priority_(0),
      stop_flag_(false),
      transmission_running_(false),
      transmission_type_(WSPR_2) // default transmission type
{
    // No processing at instantiation.
}

WSPR_Scheduler::~WSPR_Scheduler()
{
    stop(); // Ensure threads are stopped before destruction.
}

void WSPR_Scheduler::set_thread_priority(int policy, int priority)
{
    std::lock_guard<std::mutex> lock(mtx_);
    thread_policy_ = policy;
    thread_priority_ = priority;
    // Optionally, update scheduling for any already running threads.
}

void WSPR_Scheduler::start(TransmissionType type, std::function<void()> transmissionCompleteCallback)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (monitor_thread_.joinable() || transmission_running_)
    {
        llog.logS(INFO, "Transmission already running.");
        return;
    }
    stop_flag_ = false;
    transmission_type_ = type;
    transmissionCompleteCallback_ = transmissionCompleteCallback;
    // Start the monitor thread that will trigger transmission at scheduled times.
    monitor_thread_ = std::thread(&WSPR_Scheduler::monitor, this);
    apply_thread_priority(monitor_thread_);
}

void WSPR_Scheduler::stop()
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        stop_flag_ = true;
        cv_.notify_all();
    }
    if (monitor_thread_.joinable())
    {
        monitor_thread_.join();
    }
    if (transmission_thread_.joinable())
    {
        transmission_thread_.join();
    }
}

void WSPR_Scheduler::notify_complete()
{
    std::function<void()> callback;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        transmission_running_ = false;
        cv_.notify_all();
        // Copy the callback to invoke it outside the lock.
        callback = transmissionCompleteCallback_;
    }
    if (callback)
    {
        callback();
    }
}

void WSPR_Scheduler::monitor()
{
    while (true)
    {
        // Compute the next scheduled time based on the transmission type.
        auto next_schedule = compute_next_schedule(transmission_type_);
        {
            std::unique_lock<std::mutex> lock(mtx_);
            // Wait (without using sleep) until the scheduled time or a stop request.
            if (cv_.wait_until(lock, next_schedule, [this]()
                               { return stop_flag_.load(); }))
            {
                break; // stop_flag_ triggered.
            }
        }
        if (stop_flag_.load())
            break;

        // Ensure no transmission is currently running.
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (transmission_running_)
            {
                // Skip starting a new transmission if one is still running.
                continue;
            }
            transmission_running_ = true;
        }

        // Start the appropriate transmission thread if enabled.
        if (enabled_.load())
        {
            if (transmission_type_ == WSPR_2)
            {
                transmission_thread_ = std::thread(&WSPR_Scheduler::transmit_wspr2, this);
            }
            else
            {
                transmission_thread_ = std::thread(&WSPR_Scheduler::transmit_wspr15, this);
            }
            apply_thread_priority(transmission_thread_);
        }
        else
        {
            llog.logS(DEBUG, "Skipping transmission.");
        }

        // Wait for the transmission thread to complete.
        if (transmission_thread_.joinable())
        {
            transmission_thread_.join();
        }
    }
}

std::chrono::system_clock::time_point WSPR_Scheduler::compute_next_schedule(TransmissionType type)
{
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    struct tm local_tm;
#ifdef _WIN32
    localtime_s(&local_tm, &tt);
#else
    local_tm = *std::localtime(&tt);
#endif
    local_tm.tm_sec = 1; // scheduled second is always 1.
    if (type == WSPR_2)
    {
        // For WSPR_2: next even minute.
        if (local_tm.tm_min % 2 != 0)
        {
            local_tm.tm_min += 1; // make it even.
        }
        auto scheduled = std::chrono::system_clock::from_time_t(std::mktime(&local_tm));
        if (scheduled <= now)
        {
            scheduled += std::chrono::minutes(2);
        }
        return scheduled;
    }
    else
    { // type == WSPR_15
        int remainder = local_tm.tm_min % 15;
        if (remainder != 0)
        {
            local_tm.tm_min += (15 - remainder);
        }
        auto scheduled = std::chrono::system_clock::from_time_t(std::mktime(&local_tm));
        if (scheduled <= now)
        {
            scheduled += std::chrono::minutes(15);
        }
        return scheduled;
    }
}

void WSPR_Scheduler::transmit_wspr2()
{
    send_ws_message("transmit", "starting");
    llog.logS(INFO, "Starting a simulated WSPR_2 transmission.");
    auto start_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::seconds(110);
    {
        // Release the lock before calling notify_complete()
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait_until(lock, start_time + duration, [this]() { return stop_flag_.load(); });
    }
    llog.logS(INFO, "WSPR_2 transmission ending.");
    notify_complete();
    send_ws_message("transmit", "finished");
}

void WSPR_Scheduler::transmit_wspr15()
{
    send_ws_message("transmit", "starting");
    llog.logS(INFO, "Starting a simulated WSPR_15 transmission.");
    auto start_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::seconds(825);
    {
        // Release the lock before calling notify_complete()
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait_until(lock, start_time + duration, [this]() { return stop_flag_.load(); });
    }
    llog.logS(INFO, "WSPR_15 transmission ending.");
    notify_complete();
    send_ws_message("transmit", "finished");
}

void WSPR_Scheduler::apply_thread_priority(std::thread &t)
{
    if (t.joinable())
    {
        pthread_t handle = t.native_handle();
        sched_param sch_params;
        sch_params.sched_priority = thread_priority_;
        pthread_setschedparam(handle, thread_policy_, &sch_params);
    }
}

/**
 * @brief Provides informaiton that transmission is active
 *
 * @return True if a transmission is active, false otherwise.
 */
bool WSPR_Scheduler::isTransmitting() const
{
    return transmission_running_.load();
}

/**
 * @brief Sets transmission active or inactive.
 *
 * @return True if a transmission is active, false otherwise.
 */
void WSPR_Scheduler::setEnabled(bool enabled)
{
    enabled_.store(enabled);
}

void WSPR_Scheduler::send_ws_message(std::string type, std::string state)
{
    // Build JSON
    nlohmann::json j;
    j["type"] = type;
    j["state"] = state;

    // Get current time as UTC and format as ISO-8601
    auto now = std::chrono::system_clock::now();
    auto now_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_utc;
    gmtime_r(&now_t, &tm_utc);
    std::ostringstream oss;
    oss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");
    j["timestamp"] = oss.str();

    // Serialize and send
    std::string message = j.dump();
    socketServer.send_to_client(message);
}
