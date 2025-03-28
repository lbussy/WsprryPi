#ifndef _WSPR_H
#define _WSPR_H

#include "wspr_message.hpp"

extern void cleanup_dma();
extern void setup_dma();
extern void transmit_tone(double test_tone_frequency, double ppm);
extern void transmit_wspr(WsprMessage message, double wspr_frequency, double ppm, bool use_offset);

#endif // _WSPR_H
