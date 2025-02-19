/**
 * @file main.cpp
 * @brief
 *
 * This file is part of WsprryPi, forked from threeme3/WsprryPi (no longer
 * active).
 *
 * Copyright (C) @threeme3 (unknown dates)
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * WsprryPi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>     // transform support
#include <vector>        // vector support
#include <assert.h>      // 'assert' support
#include <fcntl.h>       // O_RDWR support
#include <getopt.h>      // getopt_long support
#include <iterator>      // istream_iterator support
#include <math.h>        // NAN support
#include <signal.h>      // Sig interrupt support
#include <stdexcept>     // no_argument, required_argument support
#include <sys/mman.h>    // PROT_READ, PROT_WRITE, MAP_FAILED support
#include <sys/time.h>    // gettimeofday support
#include <sys/timex.h>   // ntp_adjtime, TIME_OK support
#include <cctype>        // isdigit() support
#include <cstdlib>       // strtod() support
#include <string>        // string support
#include <unordered_map> // unordered_map support
#include <iostream>
#include <termios.h>
#include <unistd.h>

#include "lcblog.hpp"       // Submodule path included in Makefile
#include "ini_file.hpp"     // Submodule path included in Makefile
#include "wspr_message.hpp" // Submodule path included in Makefile
#include "version.hpp"
#include "monitorfile.hpp"
#include "singleton.hpp"
#include "arg_parser.hpp"
#include "constants.hpp"
// #include "hardware_access.hpp"

#include "main.hpp"

/**
 * @var original_term
 * @brief Stores the original terminal settings.
 * @details This structure is used to restore the terminal settings
 *          before the program exits.
 */
static struct termios original_term;

// Set with "make debug"
// #define WSPR_DEBUG

// Logging library
LCBLog llog;

// INI File Library
IniFile ini;

// Class to monitor for file changes
MonitorFile iniMonitor;

// Note on accessing memory in RPi:
//
// There are 3 (yes three) address spaces in the Pi:
// Physical addresses
//   These are the actual address locations of the RAM and are equivalent
//   to offsets into /dev/mem.
//   The peripherals (DMA engine, PWM, etc.) are located at physical
//   address 0x2000000 for RPi1 and 0x3F000000 for RPi2/3.
// Virtual addresses
//   These are the addresses that a program sees and can read/write to.
//   Addresses 0x00000000 through 0xBFFFFFFF are the addresses available
//   to a program running in user space.
//   Addresses 0xC0000000 and above are available only to the kernel.
//   The peripherals start at address 0xF2000000 in virtual space but
//   this range is only accessible by the kernel. The kernel could directly
//   access peripherals from virtual addresses. It is not clear to me my
//   a user space application running as 'root' does not have access to this
//   memory range.
// Bus addresses
//   This is a different (virtual?) address space that also maps onto
//   physical memory.
//   The peripherals start at address 0x7E000000 of the bus address space.
//   The DRAM is also available in bus address space in 4 different locations:
//   0x00000000 "L1 and L2 cached alias"
//   0x40000000 "L2 cache coherent (non allocating)"
//   0x80000000 "L2 cache (only)"
//   0xC0000000 "Direct, uncached access"
//
// Accessing peripherals from user space (virtual addresses):
//   The technique used in this program is that mmap is used to map portions of
//   /dev/mem to an arbitrary virtual address. For example, to access the
//   GPIO's, the gpio range of addresses in /dev/mem (physical addresses) are
//   mapped to a kernel chosen virtual address. After the mapping has been
//   set up, writing to the kernel chosen virtual address will actually
//   write to the GPIO addresses in physical memory.
//
// Accessing RAM from DMA engine
//   The DMA engine is programmed by accessing the peripheral registers but
//   must use bus addresses to access memory. Thus, to use the DMA engine to
//   move memory from one virtual address to another virtual address, one needs
//   to first find the physical addresses that corresponds to the virtual
//   addresses. Then, one needs to find the bus addresses that corresponds to
//   those physical addresses. Finally, the DMA engine can be programmed. i.e.
//   DMA engine access should use addresses starting with 0xC.
//
// The perhipherals in the Broadcom documentation are described using their bus
// addresses and structures are created and calculations performed in this
// program to figure out how to access them with virtual addresses.

// Given an address in the bus address space of the peripherals, this
// macro calculates the appropriate virtual address to use to access
// the requested bus address space. It does this by first subtracting
// 0x7e000000 from the supplied bus address to calculate the offset into
// the peripheral address space. Then, this offset is added to peri_base_virt
// Which is the base address of the peripherals, in virtual address space.
#define ACCESS_BUS_ADDR(buss_addr) *(volatile int *)((long int)peri_base_virt + (buss_addr) - 0x7e000000)
// Given a bus address in the peripheral address space, set or clear a bit.
#define SETBIT_BUS_ADDR(base, bit) ACCESS_BUS_ADDR(base) |= 1 << bit
#define CLRBIT_BUS_ADDR(base, bit) ACCESS_BUS_ADDR(base) &= ~(1 << bit)

// Used for GPIO DIO Access:
// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio + ((g) / 10)) &= ~(7 << (((g) % 10) * 3))
#define OUT_GPIO(g) *(gpio + ((g) / 10)) |= (1 << (((g) % 10) * 3))
#define SET_GPIO_ALT(g, a) *(gpio + (((g) / 10))) |= (((a) <= 3 ? (a) + 4 : (a) == 4 ? 3  \
                                                                                     : 2) \
                                                      << (((g) % 10) * 3))
#define GPIO_SET *(gpio + 7)                  // sets bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio + 10)                 // clears bits which are 1 ignores bits which are 0
#define GET_GPIO(g) (*(gpio + 13) & (1 << g)) // 0 if LOW, (1<<g) if HIGH
#define GPIO_PULL *(gpio + 37)                // Pull up/pull down
#define GPIO_PULLCLK0 *(gpio + 38)            // Pull up/pull down clock

// Convert from a bus address to a physical address.
#define BUS_TO_PHYS(x) ((x) & ~0xC0000000)

// peri_base_virt is the base virtual address that a userspace program (this
// program) can use to read/write to the the physical addresses controlling
// the peripherals. This address is mapped at runtime using mmap and /dev/mem.
// This must be declared global so that it can be called by the atexit
// function.
volatile unsigned *peri_base_virt = NULL;

// Used for GPIO DIO Access:
// Map to GPIO register
void *gpio_map;
// I/O access
volatile unsigned *gpio;

struct GPCTL
{
    // Structure used to control clock generator
    char SRC : 4;
    char ENAB : 1;
    char KILL : 1;
    char : 1;
    char BUSY : 1;
    char FLIP : 1;
    char MASH : 2;
    unsigned int : 13;
    char PASSWD : 8;
};

struct CB
{
    // Structure used to tell the DMA engine what to do
    volatile unsigned int TI;
    volatile unsigned int SOURCE_AD;
    volatile unsigned int DEST_AD;
    volatile unsigned int TXFR_LEN;
    volatile unsigned int STRIDE;
    volatile unsigned int NEXTCONBK;
    volatile unsigned int RES1;
    volatile unsigned int RES2;
};

struct DMAregs
{
    // DMA engine status registers
    volatile unsigned int CS;
    volatile unsigned int CONBLK_AD;
    volatile unsigned int TI;
    volatile unsigned int SOURCE_AD;
    volatile unsigned int DEST_AD;
    volatile unsigned int TXFR_LEN;
    volatile unsigned int STRIDE;
    volatile unsigned int NEXTCONBK;
    volatile unsigned int DEBUG;
};

struct PageInfo
{
    // Virtual and bus addresses of a page of physical memory.
    void *b; // bus address
    void *v; // virtual address
};

static struct
{
    // Must be global so that exit handlers can access this.
    int handle;                                          /* From mbox_open() */
    unsigned mem_ref = 0;                                /* From mem_alloc() */
    unsigned bus_addr;                                   /* From mem_lock() */
    unsigned char *virt_addr = NULL; /* From mapmem() */ // ha7ilm: originally uint8_t
    unsigned pool_size;
    unsigned pool_cnt;
} mbox;

// GPIO/DIO Control:

void setupGPIO(int pin = 0)
{
    if (pin == 0)
        return;
    // Set up gpio pointer for direct register access
    int mem_fd;
    // Set up a memory regions to access GPIO
    unsigned gpio_base = get_peripheral_address() + 0x200000;

    if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
    {
        llog.logE(FATAL, "Unable to open /dev/mem (running as root?)");
        std::exit(EXIT_FAILURE);
    }

    /* mmap GPIO */
    gpio_map = mmap(
        NULL,                   // Any adddress in our space will do
        BLOCK_SIZE,             // Map length
        PROT_READ | PROT_WRITE, // Enable reading & writting to mapped memory
        MAP_SHARED,             // Shared with other processes
        mem_fd,                 // File to map
        gpio_base               // Offset to GPIO peripheral
    );

    close(mem_fd); // No need to keep mem_fd open after mmap

    if (gpio_map == MAP_FAILED)
    {
        printf("Fail: mmap error %d\n", (int)gpio_map); // errno also set
        std::exit(EXIT_FAILURE);
    }

    // Always use volatile pointer
    gpio = (volatile unsigned *)gpio_map;

    // Set GPIO pins to output
    // Must use INP_GPIO before we can use OUT_GPIO
    INP_GPIO(pin);
    OUT_GPIO(pin);
}

void pinHigh(int pin = 0)
{
    if (pin == 0)
        return;
    GPIO_SET = 1 << pin;
}

void pinLow(int pin = 0)
{
    if (pin == 0)
        return;
    GPIO_CLR = 1 << pin;
}

// GPIO/DIO Control^

void getPLLD()
{
    // Nominal clock frequencies
    // double f_xtal = 19200000.0;
    // PLLD clock frequency.
    // For RPi1, after NTP converges, these is a 2.5 PPM difference between
    // the PPM correction reported by NTP and the actual frequency offset of
    // the crystal. This 2.5 PPM offset is not present in the RPi2 and RPi3 (RPI4).
    // This 2.5 PPM offset is compensated for here, but only for the RPi1.

    int procType = getProcessorTypeAsInt();
    switch (procType)
    {
    case 0: // RPi1
        config.mem_flag = 0x0c;
        config.f_plld_clk = (500000000.0 * (1 - 2.500e-6));
        break;
    case 1: // RPi2
    case 2: // RPi3
        config.mem_flag = 0x04;
        config.f_plld_clk = (500000000.0);
        break;
    case 3: // RPi 4
        config.mem_flag = 0x04;
        config.f_plld_clk = (750000000.0);
        break;
    default:
        fprintf(stderr, "Error: Unknown chipset (%d).", procType);
        std::exit(EXIT_FAILURE);
    }
}

void allocMemPool(unsigned numpages)
{
    // Use the mbox interface to allocate a single chunk of memory to hold
    // all the pages we will need. The bus address and the virtual address
    // are saved in the mbox structure.

    // Allocate space.
    mbox.mem_ref = mem_alloc(mbox.handle, 4096 * numpages, 4096, config.mem_flag);
    // Lock down the allocated space and return its bus address.
    mbox.bus_addr = mem_lock(mbox.handle, mbox.mem_ref);
    // Conert the bus address to a physical address and map this to virtual
    // (aka user) space.
    mbox.virt_addr = (unsigned char *)mapmem(BUS_TO_PHYS(mbox.bus_addr), 4096 * numpages);
    // The number of pages in the pool. Never changes
    mbox.pool_size = numpages;
    // How many of the created pages have actually been used.
    mbox.pool_cnt = 0;
    // printf("allocMemoryPool bus_addr=%x virt_addr=%x mem_ref=%x\n",mbox.bus_addr,(unsigned)mbox.virt_addr,mbox.mem_ref);
}

void getRealMemPageFromPool(void **vAddr, void **bAddr)
{
    // Returns the virtual and bus address (NOT physical address) of another
    // page in the pool.
    if (mbox.pool_cnt >= mbox.pool_size)
    {
        llog.logE(FATAL, "Unable to allocated more pages.");
        std::exit(EXIT_FAILURE);
    }
    unsigned offset = mbox.pool_cnt * 4096;
    *vAddr = (void *)(((unsigned)mbox.virt_addr) + offset);
    *bAddr = (void *)(((unsigned)mbox.bus_addr) + offset);
    // printf("getRealMemoryPageFromPool bus_addr=%x virt_addr=%x\n", (unsigned)*pAddr,(unsigned)*vAddr);
    mbox.pool_cnt++;
}

void deallocMemPool()
{
    // Free the memory pool
    if (mbox.virt_addr != NULL)
    {
        unmapmem(mbox.virt_addr, mbox.pool_size * 4096);
    }
    if (mbox.mem_ref != 0)
    {
        mem_unlock(mbox.handle, mbox.mem_ref);
        mem_free(mbox.handle, mbox.mem_ref);
    }
}

void disable_clock()
{
    // Disable the PWM clock and wait for it to become 'not busy'.
    // Check if mapping has been set up yet.
    if (peri_base_virt == NULL)
    {
        return;
    }
    // Disable the clock (in case it's already running) by reading current
    // settings and only clearing the enable bit.
    auto settings = ACCESS_BUS_ADDR(CM_GP0CTL_BUS);
    // Clear enable bit and add password
    settings = (settings & 0x7EF) | 0x5A000000;
    // Disable
    ACCESS_BUS_ADDR(CM_GP0CTL_BUS) = *((int *)&settings);
    // Wait for clock to not be busy.
    while (true)
    {
        if (!(ACCESS_BUS_ADDR(CM_GP0CTL_BUS) & (1 << 7)))
        {
            break;
        }
    }
}

void txon()
{
    // Turn on LED'
    if (ini.get_bool_value("Extended", "Use LED") && ini.get_int_value("Extended", "LED Pin") > 0)
        pinHigh(ini.get_int_value("Extended", "LED Pin"));
    // Set function select for GPIO4.
    // Fsel 000 => input
    // Fsel 001 => output
    // Fsel 100 => alternate function 0
    // Fsel 101 => alternate function 1
    // Fsel 110 => alternate function 2
    // Fsel 111 => alternate function 3
    // Fsel 011 => alternate function 4
    // Fsel 010 => alternate function 5
    // Function select for GPIO is configured as 'b100 which selects
    // alternate function 0 for GPIO4. Alternate function 0 is GPCLK0.
    // See section 6.2 of Arm Peripherals Manual.
    SETBIT_BUS_ADDR(GPIO_BUS_BASE, 14);
    CLRBIT_BUS_ADDR(GPIO_BUS_BASE, 13);
    CLRBIT_BUS_ADDR(GPIO_BUS_BASE, 12);

    // Set GPIO drive strength, more info: http://www.scribd.com/doc/101830961/GPIO-Pads-Control2
    switch (ini.get_int_value("Extended", "Power Level"))
    {
    case 0:
        ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 0; // 2mA -3.4dBm
        break;
    case 1:
        ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 1; // 4mA +2.1dBm
        break;
    case 2:
        ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 2; // 6mA +4.9dBm
        break;
    case 3:
        ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 3; // 8mA +6.6dBm(default)
        break;
    case 4:
        ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 4; // 10mA +8.2dBm
        break;
    case 5:
        ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 5; // 12mA +9.2dBm
        break;
    case 6:
        ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 6; // 14mA +10.0dBm
        break;
    case 7:
    default:
        ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 7; // 16mA +10.6dBm
        break;
    }

    disable_clock();

    // Set clock source as PLLD.
    struct GPCTL setupword = {6 /*SRC*/, 0, 0, 0, 0, 3, 0x5a};

    // Enable clock.
    setupword = {6 /*SRC*/, 1, 0, 0, 0, 3, 0x5a};
    ACCESS_BUS_ADDR(CM_GP0CTL_BUS) = *((int *)&setupword);
}

void txoff()
{
    // Turn off LED
    if (ini.get_bool_value("Extended", "Use LED") && ini.get_int_value("Extended", "LED Pin") > 0)
        pinLow(ini.get_int_value("Extended", "LED Pin"));
    // Turn transmitter off
    disable_clock();
}

void txSym(
    const int &sym_num,
    const double &center_freq,
    const double &tone_spacing,
    const double &tsym,
    const std::vector<double> &dma_table_freq,
    const double &f_pwm_clk,
    struct PageInfo instrs[],
    struct PageInfo &constPage,
    int &bufPtr)
{
    // Transmit symbol sym for tsym seconds.
    //
    // Note:
    // Upon entering this function at the beginning of a WSPR transmission, we
    // do not know which DMA table entry is being processed by the DMA engine.
    const int f0_idx = sym_num * 2;
    const int f1_idx = f0_idx + 1;
    const double f0_freq = dma_table_freq[f0_idx];
    const double f1_freq = dma_table_freq[f1_idx];
    const double tone_freq = center_freq - 1.5 * tone_spacing + sym_num * tone_spacing;
    // Double check...
    assert((tone_freq >= f0_freq) && (tone_freq <= f1_freq));
    const double f0_ratio = 1.0 - (tone_freq - f0_freq) / (f1_freq - f0_freq);
    // cout << "f0_ratio = " << f0_ratio << std::endl;
    assert((f0_ratio >= 0) && (f0_ratio <= 1));
    const long int n_pwmclk_per_sym = round(f_pwm_clk * tsym);

    long int n_pwmclk_transmitted = 0;
    long int n_f0_transmitted = 0;
    // printf("<instrs[bufPtr] begin=%x>",(unsigned)&instrs[bufPtr]);
    while (n_pwmclk_transmitted < n_pwmclk_per_sym)
    {
        // Number of PWM clocks for this iteration
        long int n_pwmclk = PWM_CLOCKS_PER_ITER_NOMINAL;
        // Iterations may produce spurs around the main peak based on the iteration
        // frequency. Randomize the iteration period so as to spread this peak
        // around.
        n_pwmclk += round((rand() / ((double)RAND_MAX + 1.0) - .5) * n_pwmclk) * 1;
        if (n_pwmclk_transmitted + n_pwmclk > n_pwmclk_per_sym)
        {
            n_pwmclk = n_pwmclk_per_sym - n_pwmclk_transmitted;
        }

        // Calculate number of clocks to transmit f0 during this iteration so
        // that the long term average is as close to f0_ratio as possible.
        const long int n_f0 = round(f0_ratio * (n_pwmclk_transmitted + n_pwmclk)) - n_f0_transmitted;
        const long int n_f1 = n_pwmclk - n_f0;

        // Configure the transmission for this iteration
        // Set GPIO pin to transmit f0
        bufPtr++;
        while (ACCESS_BUS_ADDR(DMA_BUS_BASE + 0x04 /* CurBlock*/) == (long int)(instrs[bufPtr].b))
            usleep(100);
        ((struct CB *)(instrs[bufPtr].v))->SOURCE_AD = (long int)constPage.b + f0_idx * 4;

        // Wait for n_f0 PWM clocks
        bufPtr++;
        while (ACCESS_BUS_ADDR(DMA_BUS_BASE + 0x04 /* CurBlock*/) == (long int)(instrs[bufPtr].b))
            usleep(100);
        ((struct CB *)(instrs[bufPtr].v))->TXFR_LEN = n_f0;

        // Set GPIO pin to transmit f1
        bufPtr++;
        while (ACCESS_BUS_ADDR(DMA_BUS_BASE + 0x04 /* CurBlock*/) == (long int)(instrs[bufPtr].b))
            usleep(100);
        ((struct CB *)(instrs[bufPtr].v))->SOURCE_AD = (long int)constPage.b + f1_idx * 4;

        // Wait for n_f1 PWM clocks
        bufPtr = (bufPtr + 1) % (1024);
        while (ACCESS_BUS_ADDR(DMA_BUS_BASE + 0x04 /* CurBlock*/) == (long int)(instrs[bufPtr].b))
            usleep(100);
        ((struct CB *)(instrs[bufPtr].v))->TXFR_LEN = n_f1;

        // Update counters
        n_pwmclk_transmitted += n_pwmclk;
        n_f0_transmitted += n_f0;
    }
    // printf("<instrs[bufPtr]=%x %x>",(unsigned)instrs[bufPtr].v,(unsigned)instrs[bufPtr].b);
}

void unSetupDMA()
{
    // Turn off (reset) DMA engine
    // Check if mapping has been set up yet.
    if (peri_base_virt == NULL)
    {
        return;
    }
    struct DMAregs *DMA0 = (struct DMAregs *)&(ACCESS_BUS_ADDR(DMA_BUS_BASE));
    DMA0->CS = 1 << 31; // reset dma controller
    txoff();
}

double bit_trunc(const double &d, const int &lsb)
{
    // Truncate at bit lsb. i.e. set all bits less than lsb to zero.
    return floor(d / pow(2.0, lsb)) * pow(2.0, lsb);
}

void setupDMATab(
    const double &center_freq_desired,
    const double &tone_spacing,
    const double &plld_actual_freq,
    std::vector<double> &dma_table_freq,
    double &center_freq_actual,
    struct PageInfo &constPage)
{
    // Program the tuning words into the DMA table.

    // Make sure that all the WSPR tones can be produced solely by
    // varying the fractional part of the frequency divider.
    center_freq_actual = center_freq_desired;
    double div_lo = bit_trunc(plld_actual_freq / (center_freq_desired - 1.5 * tone_spacing), -12) + pow(2.0, -12);
    double div_hi = bit_trunc(plld_actual_freq / (center_freq_desired + 1.5 * tone_spacing), -12);
    if (floor(div_lo) != floor(div_hi))
    {
        center_freq_actual = plld_actual_freq / floor(div_lo) - 1.6 * tone_spacing;
        std::stringstream temp;
        temp << std::setprecision(6) << std::fixed << "Center frequency has been changed to " << center_freq_actual / 1e6 << " MHz";
        llog.logS(WARN, temp.str(), " because of hardware limitations.");
    }

    // Create DMA table of tuning words. WSPR tone i will use entries 2*i and
    // 2*i+1 to generate the appropriate tone.
    double tone0_freq = center_freq_actual - 1.5 * tone_spacing;
    std::vector<long int> tuning_word(1024);
    for (int i = 0; i < 8; i++)
    {
        double tone_freq = tone0_freq + (i >> 1) * tone_spacing;
        double div = bit_trunc(plld_actual_freq / tone_freq, -12);
        if (i % 2 == 0)
        {
            div = div + pow(2.0, -12);
        }
        tuning_word[i] = ((int)(div * pow(2.0, 12)));
    }
    // Fill the remaining table, just in case...
    for (int i = 8; i < 1024; i++)
    {
        double div = 500 + i;
        tuning_word[i] = ((int)(div * pow(2.0, 12)));
    }

    // Program the table
    dma_table_freq.resize(1024);
    for (int i = 0; i < 1024; i++)
    {
        dma_table_freq[i] = plld_actual_freq / (tuning_word[i] / pow(2.0, 12));
        ((int *)(constPage.v))[i] = (0x5a << 24) + tuning_word[i];
        if ((i % 2 == 0) && (i < 8))
        {
            assert((tuning_word[i] & (~0xfff)) == (tuning_word[i + 1] & (~0xfff)));
        }
    }
}

void setupDMA(
    struct PageInfo &constPage,
    struct PageInfo &instrPage,
    struct PageInfo instrs[])
{
    // Create the memory structures needed by the DMA engine and perform initial
    // clock configuration.

    allocMemPool(1025);

    // Allocate a page of ram for the constants
    getRealMemPageFromPool(&constPage.v, &constPage.b);

    // Create 1024 instructions allocating one page at a time.
    // Even instructions target the GP0 Clock divider
    // Odd instructions target the PWM FIFO
    int instrCnt = 0;
    while (instrCnt < 1024)
    {
        // Allocate a page of ram for the instructions
        getRealMemPageFromPool(&instrPage.v, &instrPage.b);

        // Make copy instructions
        // Only create as many instructions as will fit in the recently
        // allocated page. If not enough space for all instructions, the
        // next loop will allocate another page.
        struct CB *instr0 = (struct CB *)instrPage.v;
        int i;
        for (i = 0; i < (signed)(4096 / sizeof(struct CB)); i++)
        {
            instrs[instrCnt].v = (void *)((long int)instrPage.v + sizeof(struct CB) * i);
            instrs[instrCnt].b = (void *)((long int)instrPage.b + sizeof(struct CB) * i);
            instr0->SOURCE_AD = (unsigned long int)constPage.b + 2048;
            instr0->DEST_AD = PWM_BUS_BASE + 0x18 /* FIF1 */;
            instr0->TXFR_LEN = 4;
            instr0->STRIDE = 0;
            // instr0->NEXTCONBK = (int)instrPage.b + sizeof(struct CB)*(i+1);
            instr0->TI = (1 /* DREQ  */ << 6) | (5 /* PWM */ << 16) | (1 << 26 /* no wide*/);
            instr0->RES1 = 0;
            instr0->RES2 = 0;

            // Shouldn't this be (instrCnt%2) ???
            if (i % 2)
            {
                instr0->DEST_AD = CM_GP0DIV_BUS;
                instr0->STRIDE = 4;
                instr0->TI = (1 << 26 /* no wide*/);
            }

            if (instrCnt != 0)
                ((struct CB *)(instrs[instrCnt - 1].v))->NEXTCONBK = (long int)instrs[instrCnt].b;
            instr0++;
            instrCnt++;
        }
    }
    // Create a circular linked list of instructions
    ((struct CB *)(instrs[1023].v))->NEXTCONBK = (long int)instrs[0].b;

    // set up a clock for the PWM
    ACCESS_BUS_ADDR(CLK_BUS_BASE + 40 * 4 /*PWMCLK_CNTL*/) = 0x5A000026; // Source=PLLD and disable
    usleep(1000);
    // ACCESS_BUS_ADDR(CLK_BUS_BASE + 41*4 /*PWMCLK_DIV*/)  = 0x5A002800;
    ACCESS_BUS_ADDR(CLK_BUS_BASE + 41 * 4 /*PWMCLK_DIV*/) = 0x5A002000;  // set PWM div to 2, for 250MHz
    ACCESS_BUS_ADDR(CLK_BUS_BASE + 40 * 4 /*PWMCLK_CNTL*/) = 0x5A000016; // Source=PLLD and enable
    usleep(1000);

    // set up pwm
    ACCESS_BUS_ADDR(PWM_BUS_BASE + 0x0 /* CTRL*/) = 0;
    usleep(1000);
    ACCESS_BUS_ADDR(PWM_BUS_BASE + 0x4 /* status*/) = -1; // clear errors
    usleep(1000);
    // Range should default to 32, but it is set at 2048 after reset on my RPi.
    ACCESS_BUS_ADDR(PWM_BUS_BASE + 0x10) = 32;
    ACCESS_BUS_ADDR(PWM_BUS_BASE + 0x20) = 32;
    ACCESS_BUS_ADDR(PWM_BUS_BASE + 0x0 /* CTRL*/) = -1; //(1<<13 /* Use fifo */) | (1<<10 /* repeat */) | (1<<9 /* serializer */) | (1<<8 /* enable ch */) ;
    usleep(1000);
    ACCESS_BUS_ADDR(PWM_BUS_BASE + 0x8 /* DMAC*/) = (1 << 31 /* DMA enable */) | 0x0707;

    // activate dma
    struct DMAregs *DMA0 = (struct DMAregs *)&(ACCESS_BUS_ADDR(DMA_BUS_BASE));
    DMA0->CS = 1 << 31; // reset
    DMA0->CONBLK_AD = 0;
    DMA0->TI = 0;
    DMA0->CONBLK_AD = (unsigned long int)(instrPage.b);
    DMA0->CS = (1 << 0) | (255 << 16); // enable bit = 0, clear end flag = 1, prio=19-16
}

bool wait_every(int minute)
{
    // Wait for the system clock's minute to reach one second past 'minute'
    time_t t;
    struct tm *ptm;
    for (;;)
    {
        if (config.useini && iniMonitor.changed())
        {
            // Delay and make sure the file is done changing
            usleep(500000);
            while (iniMonitor.changed())
            {
                ;
                ;
            }

            llog.logS(INFO, "INI file changed, reloading parameters.");
            validate_config_data();
            return false; // Need to reload
        }
        time(&t);
        ptm = gmtime(&t);
        if ((ptm->tm_min % minute) == 0 && ptm->tm_sec == 0)
            break;
        usleep(1000);
    }
    usleep(1000000); // Wait another second
    return true;     // OK to proceed
}

bool update_ppm()
{
    // Call ntp_adjtime() to obtain the latest calibration coefficient.

    // TODO: Review this

    struct timex ntx;
    int status;
    double ppm_new;

    ntx.modes = 0; /* only read */
    status = ntp_adjtime(&ntx);

    if (status != TIME_OK)
    {
        llog.logE(FATAL, "Clock not synchronized.");
        return false;
    }

    ppm_new = (double)ntx.freq / (double)(1 << 16); /* frequency scale */
    if (abs(ppm_new) > 200)
    {
        llog.logE(FATAL, "Absolute ppm value is greater than 200 and is being ignored.");
        return false;
    }
    else
    {
        if (ini.get_double_value("Extended", "PPM") != ppm_new)
        {
            llog.logS(INFO, "Obtained new ppm value:", ppm_new);
        }
        ini.set_double_value("Extended", "PPM", ppm_new);
        return true;
    }
}

int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1)
{
    // Return 1 if the difference is negative, otherwise 0.
    long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
    result->tv_sec = diff / 1000000;
    result->tv_usec = diff % 1000000;

    return (diff < 0);
}

void timeval_print(struct timeval *tv)
{
    // Print Time Stamp

    char buffer[30];
    time_t curtime;

    // printf("%ld.%06ld", tv->tv_sec, tv->tv_usec);
    curtime = tv->tv_sec;
    // strftime(buffer, 30, "%m-%d-%Y %T", localtime(&curtime));
    strftime(buffer, 30, "%Y-%m-%d %T", gmtime(&curtime));
    printf("%s.%03ld", buffer, (tv->tv_usec + 500) / 1000);
    std::cout << " UTC";
}

void open_mbox()
{
    // Create the mbox special files and open mbox.
    mbox.handle = mbox_open();
    if (mbox.handle < 0)
    {
        llog.logE(FATAL, "Failed to open mailbox.");
        std::exit(EXIT_FAILURE);
    }
}

void setSchedPriority(int priority)
{
    // In order to get the best timing at a decent queue size, we want the kernel
    // to avoid interrupting us for long durations.  This is done by giving our
    // process a high priority. Note, must run as super-user for this to work.
    struct sched_param sp;
    sp.sched_priority = priority;
    int ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
    if (ret)
    {
        llog.logE(INFO, "pthread_setschedparam (increase thread priority) returned non-zero:", ret);
    }
}

void setup_peri_base_virt(volatile unsigned *&peri_base_virt)
{
    // Create the memory map between virtual memory and the peripheral range
    // of physical memory.

    int mem_fd;
    unsigned gpio_base = get_peripheral_address();
    // open /dev/mem
    if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
    {
        llog.logE(FATAL, "Can't open /dev/mem");
        std::exit(EXIT_FAILURE);
    }
    peri_base_virt = (unsigned *)mmap(
        NULL,
        0x01000000, // len
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        mem_fd,
        gpio_base // base
    );
    if ((long int)peri_base_virt == -1)
    {
        llog.logE(FATAL, "peri_base_virt mmap error.");
        std::exit(EXIT_FAILURE);
    }
    close(mem_fd);
}

/**
 * @brief Restores the terminal settings to their original state.
 * @details This function restores terminal attributes that were saved
 *          before modifications. It is registered with `atexit()` and
 *          is also called in the signal handler.
 */
void restoreTerminalSettings()
{
    if (tcsetattr(STDIN_FILENO, TCSANOW, &original_term) != 0)
    {
        std::cerr << "Error: Failed to restore terminal attributes.\n";
    }
}

/**
 * @brief Disables the echoing of control characters in the terminal.
 * @details This function modifies terminal attributes to prevent control
 *          characters (like ^C) from being displayed. It ensures settings
 *          are restored on exit.
 */
void disableSignalEcho()
{
    struct termios term;

    // Get and save current terminal attributes
    if (tcgetattr(STDIN_FILENO, &term) != 0)
    {
        std::cerr << "Error: Failed to get terminal attributes.\n";
        return;
    }

    original_term = term;     // Store original settings
    term.c_lflag &= ~ECHOCTL; // Disable control character echoing

    // Apply the new settings immediately
    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) != 0)
    {
        std::cerr << "Error: Failed to set terminal attributes.\n";
    }

    // Ensure terminal settings are restored when the program exits
    atexit(restoreTerminalSettings);
}

/**
 * @brief Logs the exit message and performs cleanup before termination.
 * @details This function logs the signal type and description, restores
 *          terminal settings, and ensures a proper exit status.
 *
 * @param sig The signal number that caused the termination.
 * @param severity Log level (INFO for normal, FATAL for system errors).
 */
void handleExitSignal(int sig, int severity)
{
    std::string log_message;

    switch (sig)
    {
    case SIGINT:
        log_message = "Exiting due to interrupt (Ctrl+C).";
        break;
    case SIGTERM:
        log_message = "Exiting due to termination request (SIGTERM).";
        break;
    case SIGUSR1:
        log_message = "Exiting due to user-defined signal 1 (SIGUSR1).";
        break;
    case SIGUSR2:
        log_message = "Exiting due to user-defined signal 2 (SIGUSR2).";
        break;
    case SIGQUIT:
        log_message = "Exiting due to quit signal (SIGQUIT).";
        break;
    case SIGPWR:
        log_message = "Exiting due to power failure signal (SIGPWR).";
        break;
    case SIGCONT:
        log_message = "Exiting due to continue signal (SIGCONT).";
        break;
    case SIGKILL:
        log_message = "Exiting due to kill signal (SIGKILL).";
        break;
    case SIGSEGV:
        log_message = "Exiting due to segmentation fault (SIGSEGV).";
        break;
    case SIGBUS:
        log_message = "Exiting due to bus error (SIGBUS).";
        break;
    case SIGFPE:
        log_message = "Exiting due to floating point exception (SIGFPE).";
        break;
    case SIGILL:
        log_message = "Exiting due to illegal instruction (SIGILL).";
        break;
    case SIGHUP:
        log_message = "Exiting due to hangup signal (SIGHUP).";
        break;
    default:
        log_message = "Exiting due to unknown signal.";
        break;
    }

    // Log signal details
    llog.logS(INFO, log_message);
    llog.logS(INFO, "Signal:", sig);

    // Log error if it's a critical signal
    if (severity == FATAL)
    {
        llog.logE(FATAL, log_message);
        llog.logE(FATAL, "Signal:", sig);
    }

    // Restore terminal settings before exiting
    restoreTerminalSettings();

    // Perform necessary cleanup before exit
    if (ini.get_bool_value("Extended", "Use LED") && ini.get_int_value("Extended", "LED Pin") > 0)
        pinLow(ini.get_int_value("Extended", "LED Pin"));
    disable_clock();
    unSetupDMA();
    deallocMemPool();
    unlink(LOCAL_DEVICE_FILE_NAME);

    // Determine exit status
    exit(severity == FATAL ? EXIT_FAILURE : EXIT_SUCCESS);
}

/**
 * @brief Handles termination signals, logs exit messages, and cleans up.
 * @details This function is registered as a signal handler and calls
 *          `handleExitSignal()` with the appropriate severity level.
 *
 * @param sig The signal number received by the process.
 */
void cleanupAndExit(int sig)
{
    bool is_fatal = (sig == SIGSEGV || sig == SIGBUS || sig == SIGFPE || sig == SIGILL || sig == SIGKILL);
    handleExitSignal(sig, is_fatal ? FATAL : INFO);
}

/**
 * @brief Sets up signal handlers for safe program termination.
 *
 * This function registers handlers for various termination and error signals,
 * ensuring that the program can perform necessary cleanup before exiting.
 * The registered signals are:
 * - `SIGINT`  (Ctrl+C interrupt)
 * - `SIGTERM` (Termination request)
 * - `SIGQUIT` (Quit request)
 * - `SIGSEGV` (Segmentation fault)
 * - `SIGBUS`  (Bus error)
 * - `SIGFPE`  (Floating point exception)
 * - `SIGILL`  (Illegal instruction)
 * - `SIGHUP`  (Hangup signal, often sent when a terminal closes)
 *
 * Each of these signals is handled by the `cleanupAndExit` function, which should
 * perform any necessary cleanup before terminating the process.
 *
 * @note This function should be called at the beginning of `main()` to ensure proper signal handling.
 *
 * @see cleanupAndExit()
 */
void set_signal_handler()
{
    // Set up signal handlers
    signal(SIGINT, cleanupAndExit);
    signal(SIGTERM, cleanupAndExit);
    signal(SIGQUIT, cleanupAndExit);
    signal(SIGSEGV, cleanupAndExit);
    signal(SIGBUS, cleanupAndExit);
    signal(SIGFPE, cleanupAndExit);
    signal(SIGILL, cleanupAndExit);
    signal(SIGHUP, cleanupAndExit);
}

int main(const int argc, char *const argv[])
{
    llog.setLogLevel(DEBUG);

    set_signal_handler();

    // Modify terminal settings
    disableSignalEcho();

    // Parse command line arguments
    if (!parse_command_line(argc, argv))
        return false;
    // Show the net configuration values after ini and command line parsing
    show_config_values();
    // Check initial load
    if (!validate_config_data())
        return false;

    llog.logS(INFO, version_string());
    llog.logS(INFO, "Running on:", getRaspberryPiModel(), ".");

    getPLLD(); // Get PLLD Frequency

    // Set up GPIO if LED option is on
    if (ini.get_bool_value("Extended", "Use LED") && ini.get_int_value("Extended", "LED Pin") > 0)
        setupGPIO(ini.get_int_value("Extended", "LED Pin"));

    // Make sure we're the only wsprrypi process
    SingletonProcess singleton(SINGLETON_PORT);
    try
    {
        if (!singleton())
        {
            llog.logE(FATAL, "Process already running; see:", singleton.GetLockFileName());
            return 1;
        }
    }
    catch (const std::exception &e)
    {
        llog.logE(FATAL, "Failed to enforce singleton:", e.what());
        return 1;
    }

    setSchedPriority(30);

    // Initialize the RNG
    srand(time(NULL));

    // TODO:  Is this where #57 breaks?
    int nbands = config.center_freq_set.size();

    // Initial configuration
    struct PageInfo constPage;
    struct PageInfo instrPage;
    struct PageInfo instrs[1024];
    setup_peri_base_virt(peri_base_virt);

    // Set up DMA
    open_mbox();
    txon();
    setupDMA(constPage, instrPage, instrs);
    txoff();

    // Display the PID:
    llog.logS(INFO, "Process PID:", getpid());

    if (config.mode == ModeType::TONE)
    {
        // Test tone mode...
        double wspr_symtime = WSPR_SYMTIME;
        double tone_spacing = 1.0 / wspr_symtime;

        std::stringstream temp;
        temp << std::setprecision(6) << std::fixed << "Transmitting test tone on frequency " << config.test_tone / 1.0e6 << " MHz.";
        llog.logS(INFO, temp.str());
        llog.logS(INFO, "Press CTRL-C to exit.");

        txon();
        int bufPtr = 0;
        std::vector<double> dma_table_freq;
        // Set to non-zero value to ensure setupDMATab is called at least once.
        double ppm_prev = 123456;
        double center_freq_actual;
        while (true)
        {
            if (ini.get_bool_value("Extended", "Use NTP"))
            {
                if (!update_ppm())
                    cleanupAndExit(-1);
            }
            if (ini.get_double_value("Extended", "PPM") != ppm_prev)
            {
                setupDMATab(config.test_tone + 1.5 * tone_spacing, tone_spacing, config.f_plld_clk * (1 - ini.get_double_value("Extended", "PPM") / 1e6), dma_table_freq, center_freq_actual, constPage);
                // cout << std::setprecision(30) << dma_table_freq[0] << std::endl;
                // cout << std::setprecision(30) << dma_table_freq[1] << std::endl;
                // cout << std::setprecision(30) << dma_table_freq[2] << std::endl;
                // cout << std::setprecision(30) << dma_table_freq[3] << std::endl;
                if (center_freq_actual != config.test_tone + 1.5 * tone_spacing)
                {
                    std::stringstream temp;
                    temp << std::setprecision(6) << std::fixed << "Test tone will be transmitted on " << (center_freq_actual - 1.5 * tone_spacing) / 1e6 << " MHz due to hardware limitations." << std::endl;
                    llog.logE(INFO, temp.str());
                }
                ppm_prev = ini.get_double_value("Extended", "PPM");
            }
            txSym(0, center_freq_actual, tone_spacing, 60, dma_table_freq, F_PWM_CLK_INIT, instrs, constPage, bufPtr);
        }
    }
    else if (config.mode == ModeType::WSPR)
    {
        // WSPR mode
        for (;;)
        { // Reload Loop >
            // Initialize WSPR Message (message)
            WsprMessage message(ini.get_string_value("Common", "Call Sign"), ini.get_string_value("Common", "Grid Square"), ini.get_int_value("Common", "TX Power"));

            // Access the generated symbols
            unsigned char *symbols = message.symbols;

#ifdef WSPR_DEBUG
            // Use a string stream to concatenate symbols
            std::ostringstream symbols_stream;

            for (int i = 0; i < WsprMessage::size; ++i)
            {
                symbols_stream << static_cast<int>(message.symbols[i]);
                if (i < WsprMessage::size - 1)
                {
                    symbols_stream << ","; // Append a comma except for the last element
                }
            }

            // Print the concatenated string in one call
            llog.logS(DEBUG, "Generated WSPR symbols:", symbols_stream.str());
// std::cout << symbols_stream.str() << std::endl;
#endif

            llog.logS(INFO, "Ready to transmit (setup complete).");
            int band = 0;
            int transmit_counter = 0;
            for (;;)
            {
                // Calculate WSPR parameters for this transmission
                double center_freq_desired;
                center_freq_desired = config.center_freq_set[band];
                bool wspr15 =
                    (center_freq_desired > 137600 && center_freq_desired < 137625) ||
                    (center_freq_desired > 475800 && center_freq_desired < 475825) ||
                    (center_freq_desired > 1838200 && center_freq_desired < 1838225);
                double wspr_symtime = (wspr15) ? 8.0 * WSPR_SYMTIME : WSPR_SYMTIME;
                double tone_spacing = 1.0 / wspr_symtime;

                // Add random offset
                if ((center_freq_desired != 0) && ini.get_bool_value("Extended", "Offset"))
                {
                    center_freq_desired += (2.0 * rand() / ((double)RAND_MAX + 1.0) - 1.0) * (wspr15 ? WSPR15_RAND_OFFSET : WSPR_RAND_OFFSET);
                }

                // Status message before transmission
                std::stringstream temp;
                temp << std::setprecision(6) << std::fixed;
                temp << "Center frequency for " << (wspr15 ? "WSPR-15" : "WSPR") << " trans: " << center_freq_desired / 1e6 << " MHz.";
                llog.logS(INFO, temp.str());

                // Wait for WSPR transmission window to arrive.
                if (config.no_delay)
                {
                    llog.logS(INFO, "Transmitting immediately (not waiting for WSPR window.)");
                }
                else
                {
                    llog.logS(INFO, "Waiting for next WSPR transmission window.");
                    if (!wait_every((wspr15) ? 15 : 2))
                    {
                        // Break and reload if ini changes
                        break;
                    };
                }

                // Update crystal calibration information
                if (ini.get_bool_value("Extended", "Use NTP"))
                {
                    if (!update_ppm())
                        cleanupAndExit(-1);
                }

                // Create the DMA table for this center frequency
                std::vector<double> dma_table_freq;
                double center_freq_actual;
                if (center_freq_desired)
                {
                    setupDMATab(center_freq_desired, tone_spacing, config.f_plld_clk * (1 - ini.get_double_value("Extended", "PPM") / 1e6), dma_table_freq, center_freq_actual, constPage);
                }
                else
                {
                    center_freq_actual = center_freq_desired;
                }

                // Send the message if freq != 0 and transmission is enabled
                if (center_freq_actual && ini.get_bool_value("Control", "Transmit"))
                {
                    // Print a status message right before transmission begins.
                    llog.logS(INFO, "Transmission started.");

                    struct timeval tvBegin, sym_start, diff;
                    gettimeofday(&tvBegin, NULL);
                    int bufPtr = 0;

                    // Get Begin Timestamp
                    auto txBegin = std::chrono::high_resolution_clock::now();

                    txon();
                    for (int i = 0; i < 162; i++)
                    {
                        gettimeofday(&sym_start, NULL);
                        timeval_subtract(&diff, &sym_start, &tvBegin);
                        double elapsed = diff.tv_sec + diff.tv_usec / 1e6;
                        double sched_end = (i + 1) * wspr_symtime;
                        double this_sym = sched_end - elapsed;
                        this_sym = (this_sym < .2) ? .2 : this_sym;
                        this_sym = (this_sym > 2 * wspr_symtime) ? 2 * wspr_symtime : this_sym;
                        txSym(symbols[i], center_freq_actual, tone_spacing, sched_end - elapsed, dma_table_freq, F_PWM_CLK_INIT, instrs, constPage, bufPtr);
                    }
                    transmit_counter++;

                    // Turn transmitter off
                    txoff();

                    // Get End Timestamp
                    auto txEnd = std::chrono::high_resolution_clock::now();
                    // Calculate duration in <double> seconds
                    std::chrono::duration<double, std::milli> elapsed = (txEnd - txBegin) / 1000;
                    double num_seconds = elapsed.count();
                    llog.logS(INFO, "Transmission complete (", num_seconds, " sec.)");
                }
                else
                {
                    llog.logS(INFO, "Skipping transmission.");
                    usleep(1000000);
                }

                // Advance to next band
                band = (band + 1) % nbands;
                if ((band == 0) && !config.repeat)
                {
                    return 0;
                }
                // If config.terminate has a value, it's > 0, and counter is >= config.terminate (we finished our run then retur)
                if (config.terminate.has_value() && (config.terminate.value() > 0) && (transmit_counter >= config.terminate.value()))
                {
                    return 0;
                }
            }
        } // < Reload Loop
    }
    else
    {
        llog.logE(FATAL, "Mode must be either WSPR or TONE.");
        std::exit(EXIT_FAILURE);
    }

    return 0;
}

// Have to do:
// TODO: Documentation needs updates from/related to changes to command line parsing (including INI vs option precedence)
// TODO: Obtained new ppm value: -11.5494 - not working
// TODO: Reload on ini change not working

// Nice to do:
// TODO: Add in tcp server
// TODO: Consider an external file for band to frequency lookups
// TODO: Modern C++ prefers constexpr over preprocessor macros (#define). Maybe all of them can go in a separate file.
// TODO: See if we can use C++ 20 and .contains() (in arg parsing)
// TODO: Replace manual trimming – Use std::erase_if() (C++20) instead of manually erasing whitespace.
// TODO: DMA notes at: https://github.com/fandahao17/Raspberry-Pi-DMA-Tutorial
