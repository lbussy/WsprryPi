#ifndef _WSPR_H
#define _WSPR_H

#include <atomic>
#include <string>
#include <vector>

// Global stop flag.
extern std::atomic<bool> g_stop;

void dma_cleanup();
void setup_dma();
void transmit_wspr(
    double frequency,
    int power = 7,
    double ppm = 0.0,
    std::string callsign = "",
    std::string grid_square = "",
    int power_dbm = 0,
    bool use_offset = false);

#endif // _WSPR_H
