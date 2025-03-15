#ifndef _WSPR_H
#define _WSPR_H

#include "ppm_manager.hpp"

extern PPMManager ppmManager;

#ifdef __cplusplus
extern "C"
{
#include "mailbox.h"
}
#endif /* __cplusplus */

void dma_cleanup();
void setup_dma();
void tx_tone();
void tx_wspr();

#endif // _WSPR_H
