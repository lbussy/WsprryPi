#ifndef _DMA_HANDLER_HPP
#define _DMA_HANDLER_HPP

#include "wspr_message.hpp"

#ifndef _MAILBOX_H
#define _MAILBOX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mailbox.h"

#ifdef __cplusplus
}
#endif

void dma_prep();
double dma_setup(double center_freq_desired);
void tx_wspr(WsprMessage message, double center_freq_actual);
void tx_tone(double tone_frequency);

#endif // _MAILBOX_H

#endif // _DMA_HANDLER_HPP
