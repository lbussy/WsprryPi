#ifndef _WSPR_H
#define _WSPR_H

#include <atomic>
#include <string>
#include <vector>

// Global stop flag.
extern std::atomic<bool> g_stop;

void dma_cleanup();
void setup_dma();
void transmit_tone(double frequency);
void transmit_wspr(std::string callsign, std::string grid_square, int power_dbm, double frequency, bool use_offset);

#endif // _WSPR_H
