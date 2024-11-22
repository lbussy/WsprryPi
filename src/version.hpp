#ifndef _VERSION_H
#define _VERSION_H

// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

/*
 * WsprryPi
 * Updated and maintained by Lee C. Bussy
 *
 * Originally forked from threeme3/WsprryPi (no longer active), this
 * project has been significantly updated, improved, and documented for
 * ease of use.
 *
 * Inspired by a conversation with Bruce Raymond of TAPR, this fork has
 * diverged substantially from its origins and operates as an independent
 * project.
 *
 * Contributors:
 *   - threeme3 (Original Author)
 *   - Bruce Raymond (Inspiration and Guidance)
 *   - Lee Bussy, aa0nt@arrl.net
 *
 * Copyright (C) 2023-2024 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-55ad7f3 [fix_57].
*/

#include <bcm_host.h>

#define stringify(x) #x
#define macro_to_string(x) stringify(x)

const char *exeversion() { return macro_to_string(MAKE_SRC_TAG); }
const char *branch() { return macro_to_string(MAKE_SRC_BRH); }

int ver();
const char* RPiVersion();
unsigned gpioBase();

int ver()
{
    // See: https://github.com/raspberrypi/firmware/blob/5325b7802ca90ac5c87440e6acbc37c96f08b054/opt/vc/include/bcm_host.h#L93-L99
    return bcm_host_get_processor_id();
}

const char* RPiVersion()
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
