#ifndef _UTILS_HPP
#define _UTILS_HPP

#include <atomic>

extern std::atomic<bool> g_stop;

double get_ppm_from_chronyc();
void waitForOneSecondPastEvenMinute();

#endif // _UTILS_HPP