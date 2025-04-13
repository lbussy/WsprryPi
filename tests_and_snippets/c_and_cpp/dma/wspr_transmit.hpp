#ifndef _WSPR_H
#define _WSPR_H

#include <atomic>
#include <string>
#include <vector>

// Global stop flag.
extern std::atomic<bool> g_stop;

void dma_cleanup();
void setup_dma();
void tx_tone();
void tx_wspr();

#endif // _WSPR_H
