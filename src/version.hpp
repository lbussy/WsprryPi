#ifndef _VERSION_H
#define _VERSION_H
#pragma once

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
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.2-0e626d8 [1.2.2_devel].
 */

#define stringify(x) #x
#define macro_to_string(x) stringify(x)

const char *exeversion() { return macro_to_string(MAKE_SRC_TAG); }
const char *branch() { return macro_to_string(MAKE_SRC_BRH); }

int ver();
const char *RPiVersion();
unsigned gpioBase();

/* returns the processor ID
 */
#define BCM_HOST_PROCESSOR_BCM2835 0
#define BCM_HOST_PROCESSOR_BCM2836 1
#define BCM_HOST_PROCESSOR_BCM2837 2
#define BCM_HOST_PROCESSOR_BCM2838 3 /* Deprecated name */
#define BCM_HOST_PROCESSOR_BCM2711 3

static int read_string_from_file(const char *filename, const char *format, unsigned int *value)
{
    FILE *fin;
    char str[256];
    int found = 0;

    fin = fopen(filename, "rt");

    if (fin == NULL)
        return 0;

    while (fgets(str, sizeof(str), fin) != NULL)
    {
        if (value)
        {
            if (sscanf(str, format, value) == 1)
            {
                found = 1;
                break;
            }
        }
        else
        {
            if (!strcmp(str, format))
            {
                found = 1;
                break;
            }
        }
    }

    fclose(fin);

    return found;
}

static unsigned int get_revision_code()
{
    static unsigned int revision_num = ~0u; // all bits set
    unsigned int num;
    if (revision_num == ~0u && read_string_from_file("/proc/cpuinfo", "Revision : %x", &num))
        revision_num = num;
    return revision_num;
}

int bcm_host_get_processor_id(void)
{
    unsigned int revision_num = get_revision_code();

    if (revision_num & 0x800000)
    {
        return (revision_num & 0xf000) >> 12;
    }
    else
    {
        // Old style number only used 2835
        return BCM_HOST_PROCESSOR_BCM2835;
    }
}

int ver()
{
    // See: https://github.com/raspberrypi/firmware/blob/5325b7802ca90ac5c87440e6acbc37c96f08b054/opt/vc/include/bcm_host.h#L93-L99
    return bcm_host_get_processor_id();
}

const char *RPiVersion()
{
    const char *vertext[4] = {
        "Raspberry Pi 1 or Zero Model (BCM2835)",
        "Raspberry Pi 2B (BCM2836)",
        "Raspberry Pi 2B or 3B (BCM2837)",
        "Raspberry Pi 4 (BCM2711)"};
    return vertext[ver()];
}

static unsigned get_dt_ranges(const char *filename, unsigned offset)
{
    unsigned address = ~0;
    FILE *fp = fopen(filename, "rb");
    if (fp)
    {
        unsigned char buf[4];
        fseek(fp, offset, SEEK_SET);
        if (fread(buf, 1, sizeof buf, fp) == sizeof buf)
            address = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3] << 0;
        fclose(fp);
    }
    return address;
}

unsigned bcm_host_get_peripheral_address(void)
{
    unsigned address = get_dt_ranges("/proc/device-tree/soc/ranges", 4);
    if (address == 0)
        address = get_dt_ranges("/proc/device-tree/soc/ranges", 8);
    return address == ~0u ? 0x20000000 : address;
}

unsigned gpioBase()
{
    return bcm_host_get_peripheral_address();
}

#endif // _VERSION_H
