#ifndef _VERSION_H
#define _VERSION_H

#include <bcm_host.h>

int ver();
const char* version();
unsigned gpioBase();

int ver()
{
    // Return version ID
    // See: https://github.com/raspberrypi/firmware/blob/5325b7802ca90ac5c87440e6acbc37c96f08b054/opt/vc/include/bcm_host.h#L93-L99
    return bcm_host_get_processor_id();
}

const char* version()
{
    const char* vertext[4] = {
                "Raspberry Pi 1 or Zero Model (BCM2835)",
                "Raspberry Pi 2B (BCM2836)",
                "Raspberry Pi 2B or 3B (BCM2837)",
                "Raspberry Pi 4 (BCM2711)"
            };
    return vertext[ver()];
}

unsigned gpioBase()
{
    return bcm_host_get_peripheral_address();
}

#endif // _VERSION_H
