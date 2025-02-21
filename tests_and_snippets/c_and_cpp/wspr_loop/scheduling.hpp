#ifndef _SCHEDULING_HPP
#define _SCHEDULING_HPP

#include <string>
#include <ctime>
#include <vector>
#include <thread>
#include <atomic>

extern std::vector<std::thread> active_threads;
extern std::atomic<bool> exit_scheduler;

extern std::string get_current_governor();
extern bool set_cpu_governor(const std::string &governor);
extern void enable_performance_mode();
extern void restore_governor();
extern void set_scheduling_priority();
extern void set_transmission_realtime();
extern void precise_sleep_until(const timespec &target);
extern void scheduler_thread();
extern bool wait_every(int interval);
extern void wspr_loop();
extern void check_and_restore_governor();

#endif // _SCHEDULING_HPP