#ifndef _WSPR_H
#define _WSPR_H

#include <atomic>
#include <string>
#include <vector>

// Global stop flag.
extern std::atomic<bool> g_stop;

void dma_cleanup();
void transmit_wspr(double frequency, std::string callsign = "", std::string grid_square = "", int power_dbm = 0, bool use_offset = false);

#endif // _WSPR_H
