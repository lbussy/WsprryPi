/**
 * @file wspr_structs.hpp
 * @brief A set of structures used for the WSPR Transmit functionality.
 *
 * This file is part of WsprryPi, a project originally created from @threeme3
 * WsprryPi projet (no longer on GitHub). However, now the original code
 * remains only as a memory and inspiration, and this project is no longer
 * a derivative work.
 *
 * This project is is licensed under the MIT License. See LICENSE.MIT.md
 * for more information.
 *
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef WSPR_STRUCTS
#define WSPR_STRUCTS

#include "wspr_message.hpp"

#include <vector>

typedef enum
{
    WSPR,
    TONE
} mode_type;

struct wConfig
{
    // Global configuration items from command line and ini file
    bool useini = false;
    std::string inifile = "";                   // Default to empty, meaning no INI file specified
    bool xmit_enabled = true;                   // Transmission disabled by default
    bool repeat = false;                        // No repeat transmission by default
    std::string callsign = "AA0NT";             // Default to empty, requiring user input
    std::string locator = "EM18";               // Default to empty, requiring user input
    std::string frequency_string = "7040100.0"; // Default to empty
    std::vector<double> center_freq_set = {};   // Empty vector, frequencies to be defined
    double ppm = 12.136;                        // Default to zero, meaning no frequency correction applied
    bool self_cal = true;                       // Self-calibration enabled by default
    bool random_offset = true;                  // No random offset by default
    double test_tone = 7040100.0;               // Default to NAN, meaning no test tone
    bool no_delay = false;                      // Delay enabled by default
    mode_type mode = WSPR;                      // Default mode is WSPR
    int terminate = -1;                         // -1 to indicate no termination signal
    bool useled = false;                        // No LED signaling by default
    bool daemon_mode = false;                   // Not running as a daemon by default
    double f_plld_clk = 125e6;                  // Default PLLD clock frequency: 125 MHz
    int mem_flag = 0;                           // Default memory flag set to 0
} config;

struct DMAConfig
{
    double plld_clock_frequency; ///< Clock speed (defaults to 500 MHz).
    int mem_flag;                ///< Memory management flags.
    /*
    peri_base_virt is the base virtual address that a userspace program (this
    program) can use to read/write to the the physical addresses controlling
    the peripherals. This address is mapped at runtime using mmap and /dev/mem.
    This must be declared global so that it can be called by the atexit
    function.
     */
    volatile unsigned *peri_base_virt;
    /*
    Drive strength for GPIO. The drive strength value is determined by the
    following mapping:
      - 0: 2mA (-3.4dBm)
      - 1: 4mA (+2.1dBm)
      - 2: 6mA (+4.9dBm)
      - 3: 8mA (+6.6dBm) (Broadcom default)
      - 4: 10mA (+8.2dBm)
      - 5: 12mA (+9.2dBm)
      - 6: 14mA (+10.0dBm) (Default for this project)
      */
    int tx_power;

    DMAConfig()
        : plld_clock_frequency(500000000.0 * (1 - 2.500e-6)), ///< Apply 2.5 PPM correction)
          mem_flag(0x0c),                                     ///< Memory flag used for DMA
          peri_base_virt(),                                   ///< Peripherals base address
          tx_power(0)                                         ///< Default GPIO power
    {
    }
};

/**
 * @brief Global configuration object.
 *
 * This DMAConfig instance holds the transmission functionality global objects.
 */
DMAConfig dmaConfig;

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

/**
 * @brief Control structure for the clock generator.
 *
 * This structure is used to configure the clock generator on the Raspberry Pi,
 * which is essential for transmitting radio signals by manipulating the PLLD clock
 * and routing the output to a GPIO pin.
 *
 * The bit-fields in this structure allow you to set the clock source, enable or disable
 * the clock, force the clock off (kill), check the busy status, flip the output phase,
 * set the MASH filter (which affects clock stability), and provide the required password
 * to modify the control registers.
 *
 * Bit-field breakdown:
 * - **SRC (4 bits):** Clock source selection.
 * - **ENAB (1 bit):** Enable bit. Set to 1 to enable the clock.
 * - **KILL (1 bit):** Kill bit. Set to 1 to force the clock off.
 * - **(1 bit reserved):** Unused.
 * - **BUSY (1 bit):** Busy status. Indicates if the clock generator is active.
 * - **FLIP (1 bit):** Flip bit. May be used for phase inversion.
 * - **MASH (2 bits):** MASH filter setting for noise shaping.
 * - **(13 bits reserved):** Unused/reserved.
 * - **PASSWD (8 bits):** Password field required to modify the clock control registers.
 */
struct GPCTL
{
    char SRC : 4;      ///< Clock source selection.
    char ENAB : 1;     ///< Enable bit: set to enable the clock.
    char KILL : 1;     ///< Kill bit: set to force the clock off.
    char : 1;          ///< Reserved bit.
    char BUSY : 1;     ///< Busy status flag.
    char FLIP : 1;     ///< Flip flag for phase inversion.
    char MASH : 2;     ///< MASH filter setting.
    unsigned int : 13; ///< Reserved bits.
    char PASSWD : 8;   ///< Password for register modifications.
};

/**
 * @brief DMA Engine Status Registers.
 *
 * This structure represents the status and control registers for the DMA engine.
 * It is used to monitor and control DMA operations such as data transfers between
 * memory and peripherals. These registers are critical when setting up and debugging
 * DMA transfers, such as those involved in transmitting radio signals by manipulating
 * the PLLD and routing output to GPIO.
 *
 * @var DMAregs::CS
 *      Control/Status register: Holds flags for starting, resetting, and error states.
 * @var DMAregs::CONBLK_AD
 *      Current control block address: Points to the next DMA control block in the chain.
 * @var DMAregs::TI
 *      Transfer Information register: Contains configuration flags and settings for the current transfer.
 * @var DMAregs::SOURCE_AD
 *      Source Address register: Specifies the memory address to read data from.
 * @var DMAregs::DEST_AD
 *      Destination Address register: Specifies the memory address to write data to.
 * @var DMAregs::TXFR_LEN
 *      Transfer Length register: Indicates the number of bytes to transfer.
 * @var DMAregs::STRIDE
 *      Stride register: Determines the address increment between consecutive transfers.
 * @var DMAregs::NEXTCONBK
 *      Next Control Block register: Contains the address of the next control block, enabling chained transfers.
 * @var DMAregs::DEBUG
 *      Debug register: Provides information useful for debugging DMA operations.
 */
struct DMAregs
{
    volatile unsigned int CS;        ///< Control/Status register.
    volatile unsigned int CONBLK_AD; ///< Address of the current control block.
    volatile unsigned int TI;        ///< Transfer Information register.
    volatile unsigned int SOURCE_AD; ///< Source address for data transfer.
    volatile unsigned int DEST_AD;   ///< Destination address for data transfer.
    volatile unsigned int TXFR_LEN;  ///< Transfer length (in bytes).
    volatile unsigned int STRIDE;    ///< Stride for address increment.
    volatile unsigned int NEXTCONBK; ///< Address of the next control block.
    volatile unsigned int DEBUG;     ///< Debug register for diagnostics.
};

/**
 * @brief Holds the bus and virtual addresses for a physical memory page.
 *
 * This structure is used to store the mapping between the bus address (used for DMA
 * and peripheral accesses) and the virtual address (used by the application) of a single
 * page of physical memory.
 *
 * @var PageInfo::b
 *      The bus address of the physical memory page.
 * @var PageInfo::v
 *      The virtual address mapped to the physical memory page.
 */
struct PageInfo
{
    void *b; ///< Bus address.
    void *v; ///< Virtual address.
};

/**
 * @brief Page information for the constant memory page.
 *
 * This global variable holds the bus and virtual addresses of the constant memory page,
 * which is used to store fixed data required for DMA operations, such as the tuning words
 * for frequency generation.
 */
struct PageInfo constPage;

/**
 * @brief Page information for the DMA instruction page.
 *
 * This global variable holds the bus and virtual addresses of the DMA instruction page,
 * where DMA control blocks (CBs) are stored. This page is used during the setup and
 * operation of DMA transfers.
 */
struct PageInfo instrPage;

/**
 * @brief Array of page information structures for DMA control blocks.
 *
 * This global array contains the bus and virtual addresses for each page used in the DMA
 * instruction chain. It holds 1024 entries, corresponding to the 1024 DMA control blocks used
 * for managing data transfers.
 */
struct PageInfo instrs[1024];

/**
 * @brief Global mailbox structure for Broadcom mailbox communication.
 *
 * This static structure stores information related to the Broadcom mailbox interface,
 * which is used for allocating, locking, and mapping physical memory for DMA operations.
 * It is declared as a file-scope static variable so that exit handlers and other parts
 * of the program can access its members.
 *
 * @var mbox::handle
 *      Mailbox handle obtained from mbox_open(), used for communication with the mailbox.
 * @var mbox::mem_ref
 *      Memory reference returned by mem_alloc(), identifying the allocated memory block.
 * @var mbox::bus_addr
 *      Bus address of the allocated memory, obtained from mem_lock().
 * @var mbox::virt_addr
 *      Virtual address mapped to the allocated physical memory via mapmem().
 * @var mbox::pool_size
 *      The total number of memory pages allocated in the pool.
 * @var mbox::pool_cnt
 *      The count of memory pages that have been allocated from the pool so far.
 */
static struct
{
    int handle;                         ///< Mailbox handle from mbox_open().
    unsigned mem_ref = 0;               ///< Memory reference from mem_alloc().
    unsigned bus_addr;                  ///< Bus address from mem_lock().
    unsigned char *virt_addr = nullptr; ///< Virtual address from mapmem().
    unsigned pool_size;                 ///< Total number of pages in the memory pool.
    unsigned pool_cnt;                  ///< Count of allocated pages from the pool.
} mbox;

/**
 * @brief Structure containing parameters for a WSPR transmission.
 *
 * This structure encapsulates all the necessary parameters required to configure
 * and execute a WSPR transmission, including the message, transmission frequency,
 * symbol time, tone spacing, and the DMA frequency lookup table.
 */
struct WsprTransmissionParams
{
    unsigned char symbols[MSG_SIZE];
    double wspr_frequency;              ///< The desired transmission frequency in Hz.
    double wspr_symtime;                ///< The WSPR symbol time (duration of each symbol) in seconds.
    double tone_spacing;                ///< The frequency spacing between adjacent tones in Hz.
    std::vector<double> dma_table_freq; ///< The DMA frequency lookup table used for transmission.

    /**
     * @brief Default constructor for WsprTransmissionParams.
     *
     * Initializes the transmission parameters with default values:
     * - The WSPR message is default-initialized.
     * - The transmission frequency, symbol time, and tone spacing are set to 0.0.
     * - The DMA frequency lookup table is initialized as an empty vector.
     */
    WsprTransmissionParams()
        : wspr_frequency(0.0), wspr_symtime(0.0), tone_spacing(0.0), dma_table_freq(1024, 0.0)
    {
    }
};

/**
 * @brief Global instance of transmission parameters for WSPR.
 *
 * This global variable holds the current settings used for a WSPR transmission,
 * including the WSPR message, transmission frequency, symbol time, tone spacing,
 * and the DMA frequency lookup table.
 */
WsprTransmissionParams transParams;

#endif // WSPR_STRUCTS
