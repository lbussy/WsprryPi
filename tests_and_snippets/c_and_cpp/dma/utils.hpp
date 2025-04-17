#ifndef _UTILS_HPP
#define _UTILS_HPP

#include <atomic>
#include <iomanip>

#include <sys/time.h>

extern std::atomic<bool> g_stop;

double get_ppm_from_chronyc();
void wait_for_trans_window();
int timeval_subtract(struct timeval *result, const struct timeval *t2, const struct timeval *t1);
std::string timeval_print(const struct timeval *tv);
void setSchedPriority(int priority);

#endif // _UTILS_HPP
