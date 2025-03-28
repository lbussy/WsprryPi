#include "wspr_transmit.hpp"

// Project
#include "wspr_message.hpp"

#ifdef __cplusplus
extern "C"
{
#include "mailbox.h"
}
#endif /* __cplusplus */

#include "wspr_message.hpp"

// C Standard Library Headers
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

// POSIX & System-Specific Headers
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timex.h>

// C++ Standard Library Headers
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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
// The peripherals in the Broadcom documentation are described using their bus
// addresses and structures are created and calculations performed in this
// program to figure out how to access them with virtual addresses.

/**
 * @brief Random frequency offset for standard WSPR transmissions.
 *
 * This constant defines the range, in Hertz, for random frequency offsets
 * applied to standard WSPR transmissions. The offset is applied symmetrically
 * around the target frequency, resulting in a random variation of ±80 Hz.
 *
 * This helps distribute transmissions within the WSPR band, reducing the
 * likelihood of overlapping signals.
 *
 * @note This offset is applicable for standard WSPR transmissions (2-minute cycles).
 *
 * @see WSPR15_RAND_OFFSET
 */
constexpr int WSPR_RAND_OFFSET = 80;

/**
 * @brief Random frequency offset for WSPR-15 transmissions.
 *
 * This constant defines the range, in Hertz, for random frequency offsets
 * applied to WSPR-15 transmissions. The offset is applied symmetrically
 * around the target frequency, resulting in a random variation of ±8 Hz.
 *
 * This ensures that WSPR-15 transmissions remain within the allocated band
 * while introducing slight variations to minimize signal collisions.
 *
 * @note This offset is specific to WSPR-15 transmissions (15-minute cycles).
 *
 * @see WSPR_RAND_OFFSET
 */
constexpr int WSPR15_RAND_OFFSET = 8;

/**
 * @brief Nominal symbol duration for WSPR transmissions.
 *
 * This constant represents the nominal time duration of a WSPR symbol,
 * calculated as 8192 samples divided by a sample rate of 12000 Hz.
 *
 * @details This duration is a key parameter in WSPR transmissions,
 * ensuring the correct timing for symbol generation and encoding.
 *
 * @note Any deviation in sample rate or processing latency could affect
 *       the actual symbol duration.
 */
constexpr double WSPR_SYMTIME = 8192.0 / 12000.0;

/**
 * @brief Initial empirical value for the PWM clock frequency used in WSPR
 * transmissions.
 *
 * This constant represents an empirically determined value for `F_PWM_CLK`
 * that produces WSPR symbols approximately 0.682 seconds long (WSPR_SYMTIME).
 *
 * @details Despite using DMA for transmission, system load on the Raspberry Pi
 * affects the exact TX symbol length. However, the main loop compensates
 * for these variations dynamically.
 *
 * @note If system performance changes, this value may require adjustment.
 */
constexpr double F_PWM_CLK_INIT = 31156186.6125761;

/**
 * @brief Base bus address for the General-Purpose Input/Output (GPIO)
 * registers.
 *
 * The GPIO peripheral bus base address is used to access GPIO-related
 * registers for pin configuration and control.
 */
constexpr uint32_t GPIO_BUS_BASE = 0x7E200000;

/**
 * @brief Bus address for the General-Purpose Clock 0 Control Register.
 *
 * This register controls the clock settings for General-Purpose Clock 0 (GP0).
 */
constexpr uint32_t CM_GP0CTL_BUS = 0x7E101070;

/**
 * @brief Bus address for the General-Purpose Clock 0 Divider Register.
 *
 * This register sets the frequency division for General-Purpose Clock 0.
 */
constexpr uint32_t CM_GP0DIV_BUS = 0x7E101074;

/**
 * @brief Bus address for the GPIO pads control register (GPIO 0-27).
 *
 * This register configures drive strength, pull-up, and pull-down settings
 * for GPIO pins 0 through 27.
 */
constexpr uint32_t PADS_GPIO_0_27_BUS = 0x7E10002C;

/**
 * @brief Base bus address for the clock management registers.
 *
 * The clock management unit controls various clock sources and divisors
 * for different peripherals on the Raspberry Pi.
 */
constexpr uint32_t CLK_BUS_BASE = 0x7E101000;

/**
 * @brief Base bus address for the Direct Memory Access (DMA) controller.
 *
 * The DMA controller allows high-speed data transfer between peripherals
 * and memory without CPU intervention.
 */
constexpr uint32_t DMA_BUS_BASE = 0x7E007000;

/**
 * @brief Base bus address for the Pulse Width Modulation (PWM) controller.
 *
 * The PWM controller is responsible for generating PWM signals used in
 * applications like audio output, LED dimming, and motor control.
 */
constexpr uint32_t PWM_BUS_BASE = 0x7E20C000;

/**
 * @brief The size of a memory page in bytes.
 *
 * Defines the standard memory page size used for memory-mapped I/O operations.
 * This value is typically 4 KB (4096 bytes) on most systems, aligning with the
 * memory management unit (MMU) requirements.
 */
constexpr std::uint32_t PAGE_SIZE = 4 * 1024;

/**
 * @brief The size of a memory block in bytes.
 *
 * Defines the standard block size for memory operations, used in memory-mapped
 * peripheral access and buffer allocation. This value matches `PAGE_SIZE` to
 * ensure proper memory alignment when mapping hardware registers.
 */
constexpr std::uint32_t BLOCK_SIZE = 4 * 1024;

/**
 * @brief The nominal number of PWM clock cycles per iteration.
 *
 * This constant defines the expected number of PWM clock cycles required for
 * a single iteration of the waveform generation process. It serves as a reference
 * value to maintain precise timing in signal generation.
 */
constexpr std::uint32_t PWM_CLOCKS_PER_ITER_NOMINAL = 1000;

struct DMAConfig
{
    double plld_clock_frequency; ///< Clock speed (defaults to 500 MHz).
    int mem_flag;      ///< Reserved for future memory management flags.
    // dmaConfig.peri_base_virt is the base virtual address that a userspace program (this
    // program) can use to read/write to the the physical addresses controlling
    // the peripherals. This address is mapped at runtime using mmap and /dev/mem.
    // This must be declared global so that it can be called by the atexit
    // function.
    volatile unsigned *peri_base_virt;

    DMAConfig()
        : plld_clock_frequency(500000000.0 * (1 - 2.500e-6)), // Apply 2.5 PPM correction),
          mem_flag(0x0c),
          peri_base_virt()
    {
    }
};

/**
 * @brief Global configuration object.
 *
 * This ArgParserConfig instance holds the application’s configuration settings,
 * typically loaded from an INI file or a JSON configuration.
 */
DMAConfig dmaConfig;

// Given an address in the bus address space of the peripherals, this
// macro calculates the appropriate virtual address to use to access
// the requested bus address space. It does this by first subtracting
// 0x7e000000 from the supplied bus address to calculate the offset into
// the peripheral address space. Then, this offset is added to dmaConfig.peri_base_virt
// Which is the base address of the peripherals, in virtual address space.
#define ACCESS_BUS_ADDR(buss_addr) *(volatile int *)((long int)dmaConfig.peri_base_virt + (buss_addr) - 0x7e000000)
// Given a bus address in the peripheral address space, set or clear a bit.
#define SETBIT_BUS_ADDR(base, bit) ACCESS_BUS_ADDR(base) |= 1 << bit
#define CLRBIT_BUS_ADDR(base, bit) ACCESS_BUS_ADDR(base) &= ~(1 << bit)

// Convert from a bus address to a physical address.
#define BUS_TO_PHYS(x) ((x) & ~0xC0000000)

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
    int handle;                      // From mbox_open()
    unsigned mem_ref = 0;            // From mem_alloc()
    unsigned bus_addr;               // From mem_lock()
    unsigned char *virt_addr = NULL; // From mapmem()
    unsigned pool_size;
    unsigned pool_cnt;
} mbox;

struct PageInfo constPage;
struct PageInfo instrPage;
struct PageInfo instrs[1024];

/// Invalid memory address marker
constexpr unsigned INVALID_ADDRESS = ~0u;

/// Processor IDs
constexpr int BCM_HOST_PROCESSOR_BCM2835 = 0; // BCM2835 (RPi1)
constexpr int BCM_HOST_PROCESSOR_BCM2836 = 1; // BCM2836 (RPi2)
constexpr int BCM_HOST_PROCESSOR_BCM2837 = 2; // BCM2837 (RPi3)
constexpr int BCM_HOST_PROCESSOR_BCM2711 = 3; // BCM2711 (RPi4)

unsigned get_device_tree_ranges(const char *filename, unsigned offset)
{
    unsigned address = ~0; ///< Default to an invalid address (all bits set).

    // Open the specified file in binary read mode.
    FILE *fp = fopen(filename, "rb");
    if (fp)
    {
        unsigned char buf[4]; ///< Buffer to hold the 4-byte address.

        // Seek to the specified offset within the file.
        if (fseek(fp, offset, SEEK_SET) == 0)
        {
            // Read 4 bytes and construct the address if successful.
            if (fread(buf, 1, sizeof(buf), fp) == sizeof(buf))
            {
                address = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
            }
        }

        // Close the file after reading.
        fclose(fp);
    }

    return address;
}

unsigned get_peripheral_address(void)
{
    // Attempt to get the peripheral address from the device tree at offset 4.
    unsigned address = get_device_tree_ranges("/proc/device-tree/soc/ranges", 4);

    // If the address is zero, try again at offset 8.
    if (address == 0)
    {
        address = get_device_tree_ranges("/proc/device-tree/soc/ranges", 8);
    }

    // If still invalid, return the default base address (0x20000000).
    return (address == static_cast<unsigned>(~0)) ? 0x20000000 : address;
}

std::optional<unsigned> read_string_from_file(const std::string &filename, const std::string &format)
{
    std::ifstream file(filename);
    if (!file)
    {
        return std::nullopt; // File could not be opened
    }

    std::string line;
    unsigned value = 0;
    while (std::getline(file, line))
    {
        if (sscanf(line.c_str(), format.c_str(), &value) == 1)
        {
            return value;
        }
    }

    return std::nullopt; // No match found
}

unsigned get_revision_code()
{
    static std::optional<unsigned> cached_revision;
    if (!cached_revision)
    {
        cached_revision = read_string_from_file("/proc/cpuinfo", "Revision : %x").value_or(0);
    }
    return *cached_revision;
}

int get_processor_id()
{
    unsigned rev = get_revision_code();
    return (rev & 0x800000) ? ((rev & 0xf000) >> 12) : BCM_HOST_PROCESSOR_BCM2835;
}

void get_plld_frequency()
{
    int processor_id = get_processor_id(); // Store processor ID to avoid redundant calls

    switch (processor_id)
    {
    case BCM_HOST_PROCESSOR_BCM2835: // Raspberry Pi 1
        dmaConfig.mem_flag = 0x0c;
        dmaConfig.plld_clock_frequency = 500000000.0 * (1 - 2.500e-6); // Apply 2.5 PPM correction
        break;
    case BCM_HOST_PROCESSOR_BCM2836: // Raspberry Pi 2
    case BCM_HOST_PROCESSOR_BCM2837: // Raspberry Pi 3
        dmaConfig.mem_flag = 0x04;
        dmaConfig.plld_clock_frequency = 500000000.0; // Standard 500 MHz clock
        break;
    case BCM_HOST_PROCESSOR_BCM2711: // Raspberry Pi 4
        dmaConfig.mem_flag = 0x04;
        dmaConfig.plld_clock_frequency = 750000000.0; // Higher 750 MHz clock
        break;
    default:
        std::cerr << "Error: Unknown chipset (" << processor_id << ")." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Ensure a valid PLLD frequency
    if (dmaConfig.plld_clock_frequency <= 0)
    {
        std::cerr << "Error: Invalid PLL clock frequency. Using default 500 MHz." << std::endl;
        dmaConfig.plld_clock_frequency = 500000000.0;
    }
}

void allocate_memory_pool(unsigned numpages)
{
    // Allocate a contiguous block of memory using the mailbox interface.
    mbox.mem_ref = mem_alloc(mbox.handle, PAGE_SIZE * numpages, BLOCK_SIZE, dmaConfig.mem_flag);

    // Lock the allocated memory block and retrieve its bus address.
    mbox.bus_addr = mem_lock(mbox.handle, mbox.mem_ref);

    // Convert the bus address to a physical address and map it to user space.
    mbox.virt_addr = static_cast<unsigned char *>(mapmem(BUS_TO_PHYS(mbox.bus_addr), PAGE_SIZE * numpages));

    // Store the total number of pages allocated in the pool (constant for its lifetime).
    mbox.pool_size = numpages;

    // Initialize the count of used pages in the pool.
    mbox.pool_cnt = 0;

    // Debug print: Displays memory allocation details in hexadecimal format.
    std::cout << "DEBUG: allocate_memory_pool bus_addr=0x"
              << std::hex << mbox.bus_addr
              << " virt_addr=0x" << reinterpret_cast<unsigned long>(mbox.virt_addr)
              << " mem_ref=0x" << mbox.mem_ref
              << std::dec << std::endl;
}

void get_real_mem_pages_from_pool(
    void **vAddr, // Pointer to store the virtual address of the allocated page.
    void **bAddr  // Pointer to store the bus address of the allocated page.
)
{
    // Ensure that we do not exceed the allocated pool size.
    if (mbox.pool_cnt >= mbox.pool_size)
    {
        std::cerr << "Error: unable to allocate more pages." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Compute the offset for the next available page.
    unsigned offset = mbox.pool_cnt * PAGE_SIZE;

    // Retrieve the virtual and bus addresses based on the offset.
    *vAddr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(mbox.virt_addr) + offset);
    *bAddr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(mbox.bus_addr) + offset);

    // Debug print: Displays allocated memory details.
    std::cout << "DEBUG: get_real_mem_pages_from_pool bus_addr=0x"
              << std::hex << reinterpret_cast<uintptr_t>(*bAddr)
              << " virt_addr=0x" << reinterpret_cast<uintptr_t>(*vAddr)
              << std::dec << std::endl;

    // Increment the count of allocated pages.
    mbox.pool_cnt++;
}

void deallocate_memory_pool()
{
    // Free virtual memory mapping if it was allocated.
    if (mbox.virt_addr != nullptr)
    {
        unmapmem(mbox.virt_addr, mbox.pool_size * PAGE_SIZE);
        mbox.virt_addr = nullptr; // Prevent dangling pointer usage
    }

    // Free the allocated memory block if it was successfully allocated.
    if (mbox.mem_ref != 0)
    {
        mem_unlock(mbox.handle, mbox.mem_ref);
        mem_free(mbox.handle, mbox.mem_ref);
        mbox.mem_ref = 0; // Ensure it does not reference a freed block
    }

    // Reset pool tracking variables
    mbox.pool_size = 0;
    mbox.pool_cnt = 0;
}

void disable_clock()
{
    // Ensure memory-mapped peripherals are initialized before proceeding.
    if (dmaConfig.peri_base_virt == nullptr)
    {
        return;
    }

    // Read current clock settings from the clock control register.
    auto settings = ACCESS_BUS_ADDR(CM_GP0CTL_BUS);

    // Disable the clock: clear the enable bit while preserving other settings.
    // Apply the required password (0x5A000000) to modify the register.
    settings = (settings & 0x7EF) | 0x5A000000;
    ACCESS_BUS_ADDR(CM_GP0CTL_BUS) = static_cast<int>(settings);

    // Wait until the clock is no longer busy.
    while (ACCESS_BUS_ADDR(CM_GP0CTL_BUS) & (1 << 7))
    {
        // Busy-wait loop to ensure clock disable is complete.
    }
}

void start_transmission()
{
    // Configure GPIO4 function select (Fsel) to alternate function 0 (GPCLK0).
    // This setting follows Section 6.2 of the ARM Peripherals Manual.
    SETBIT_BUS_ADDR(GPIO_BUS_BASE, 14); // Set bit 14
    CLRBIT_BUS_ADDR(GPIO_BUS_BASE, 13); // Clear bit 13
    CLRBIT_BUS_ADDR(GPIO_BUS_BASE, 12); // Clear bit 12

    // Set GPIO drive strength to 16mA (+10.6dBm output power).
    // Values range from 2mA (-3.4dBm) to 16mA (+10.6dBm).
    ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5A000018 + 7;
    // TODO:
    // Set GPIO drive strength, more info: http://www.scribd.com/doc/101830961/GPIO-Pads-Control2
    // ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 0;  //2mA -3.4dBm
    // ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 1;  //4mA +2.1dBm
    // ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 2;  //6mA +4.9dBm
    // ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 3;  //8mA +6.6dBm(default)
    // ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 4;  //10mA +8.2dBm
    // ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 5;  //12mA +9.2dBm
    // ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 6;  //14mA +10.0dBm

    // Ensure the clock is disabled before modifying settings.
    disable_clock();

    // Define clock control structure and set PLLD as the clock source.
    struct GPCTL setupword = {6 /*SRC*/, 0, 0, 0, 0, 3, 0x5A};

    // Enable the clock by modifying the control word.
    setupword = {6 /*SRC*/, 1, 0, 0, 0, 3, 0x5A};
    int temp;
    std::memcpy(&temp, &setupword, sizeof(int));

    // Apply clock control settings.
    ACCESS_BUS_ADDR(CM_GP0CTL_BUS) = temp;
}

void stop_transmission()
{
    // Disable the clock, effectively turning off transmission.
    disable_clock();
}

void transmit_symbol(
    const int &sym_num,                        // The symbol number to transmit.
    const double &center_freq,                 // The center frequency in Hz.
    const double &tone_spacing,                // The frequency spacing between tones in Hz.
    const double &tsym,                        // The duration (seconds) for which the symbol is transmitted.
    const std::vector<double> &dma_table_freq, // The DMA frequency lookup table.
    const double &f_pwm_clk,                   // The frequency of the PWM clock in Hz.
    struct PageInfo instrs[],                  // The DMA instruction page array.
    struct PageInfo &constPage,                // The memory page containing constant data.
    int &bufPtr                                // The buffer pointer index for DMA instruction handling.
)
{
    // Determine indices for DMA frequency table
    const int f0_idx = sym_num * 2;
    const int f1_idx = f0_idx + 1;

    // Retrieve frequency values from the DMA table
    const double f0_freq = dma_table_freq[f0_idx];
    const double f1_freq = dma_table_freq[f1_idx];

    // Calculate the expected tone frequency
    const double tone_freq = center_freq - 1.5 * tone_spacing + sym_num * tone_spacing;

    // Ensure the tone frequency is within bounds
    assert((tone_freq >= f0_freq) && (tone_freq <= f1_freq));

    // Compute frequency ratio for symbol transmission
    const double f0_ratio = 1.0 - (tone_freq - f0_freq) / (f1_freq - f0_freq);

    std::cout << "DEBUG: f0_ratio = " << f0_ratio << std::endl;
    assert((f0_ratio >= 0) && (f0_ratio <= 1));

    // Compute total number of PWM clock cycles required for the symbol duration
    const long int n_pwmclk_per_sym = std::round(f_pwm_clk * tsym);

    // Counters for tracking transmitted PWM clocks and f0 occurrences
    long int n_pwmclk_transmitted = 0;
    long int n_f0_transmitted = 0;

    std::cout << "DEBUG: <instrs[bufPtr] begin=0x"
              << std::hex << reinterpret_cast<unsigned long>(&instrs[bufPtr])
              << ">" << std::dec << std::endl;

    // Transmit the symbol using PWM clocks
    while (n_pwmclk_transmitted < n_pwmclk_per_sym)
    {
        // Set the nominal number of PWM clocks per iteration
        long int n_pwmclk = PWM_CLOCKS_PER_ITER_NOMINAL;

        // Introduce slight randomization to spread spectral spurs
        n_pwmclk += std::round((std::rand() / (static_cast<double>(RAND_MAX) + 1.0) - 0.5) * n_pwmclk);

        // Ensure we do not exceed the required clock cycles for the symbol
        if (n_pwmclk_transmitted + n_pwmclk > n_pwmclk_per_sym)
        {
            n_pwmclk = n_pwmclk_per_sym - n_pwmclk_transmitted;
        }

        // Compute the number of f0 and f1 cycles for this iteration
        const long int n_f0 = std::round(f0_ratio * (n_pwmclk_transmitted + n_pwmclk)) - n_f0_transmitted;
        const long int n_f1 = n_pwmclk - n_f0;

        // Transmit frequency f0
        bufPtr++;
        while (ACCESS_BUS_ADDR(DMA_BUS_BASE + 0x04 /* CurBlock */) == reinterpret_cast<long int>(instrs[bufPtr].b))
        {
            usleep(100);
        }
        reinterpret_cast<struct CB *>(instrs[bufPtr].v)->SOURCE_AD = reinterpret_cast<long int>(constPage.b) + f0_idx * 4;

        // Wait for f0 PWM clocks
        bufPtr++;
        while (ACCESS_BUS_ADDR(DMA_BUS_BASE + 0x04 /* CurBlock */) == reinterpret_cast<long int>(instrs[bufPtr].b))
        {
            usleep(100);
        }
        reinterpret_cast<struct CB *>(instrs[bufPtr].v)->TXFR_LEN = n_f0;

        // Transmit frequency f1
        bufPtr++;
        while (ACCESS_BUS_ADDR(DMA_BUS_BASE + 0x04 /* CurBlock */) == reinterpret_cast<long int>(instrs[bufPtr].b))
        {
            usleep(100);
        }
        reinterpret_cast<struct CB *>(instrs[bufPtr].v)->SOURCE_AD = reinterpret_cast<long int>(constPage.b) + f1_idx * 4;

        // Wait for f1 PWM clocks
        bufPtr = (bufPtr + 1) % 1024;
        while (ACCESS_BUS_ADDR(DMA_BUS_BASE + 0x04 /* CurBlock */) == reinterpret_cast<long int>(instrs[bufPtr].b))
        {
            usleep(100);
        }
        reinterpret_cast<struct CB *>(instrs[bufPtr].v)->TXFR_LEN = n_f1;

        // Update transmission counters
        n_pwmclk_transmitted += n_pwmclk;
        n_f0_transmitted += n_f0;
    }

    std::cout << "DEBUG: <instrs[bufPtr]=0x"
              << std::hex << reinterpret_cast<unsigned long>(instrs[bufPtr].v)
              << " 0x" << reinterpret_cast<unsigned long>(instrs[bufPtr].b)
              << ">" << std::dec << std::endl;
}

void reset_dma()
{
    // Ensure memory-mapped peripherals are initialized before proceeding.
    if (dmaConfig.peri_base_virt == nullptr)
    {
        return;
    }

    // Obtain a pointer to the DMA control registers.
    volatile DMAregs *DMA0 = reinterpret_cast<volatile DMAregs *>(&(ACCESS_BUS_ADDR(DMA_BUS_BASE)));

    // Reset the DMA controller by setting the reset bit (bit 31) in the control/status register.
    DMA0->CS = 1 << 31;

    // Turn off transmission.
    stop_transmission();
}

double bit_truncate(const double &d, const int &lsb)
{
    // Compute the truncation factor as a power of 2.
    const double factor = std::pow(2.0, lsb);

    // Truncate the number by dividing, flooring, and multiplying back.
    return std::floor(d / factor) * factor;
}

void setup_dma_frequency_table(
    const double &center_freq_desired,
    const double &tone_spacing,
    const double &plld_actual_freq,
    std::vector<double> &dma_table_freq,
    double &center_freq_actual,
    struct PageInfo &constPage)
{
    // Ensure DMA frequency table has a minimum size of 1024.
    if (dma_table_freq.size() < 1024)
    {
        std::cerr << "Error: DMA table frequency array not initialized properly." << std::endl;
        dma_table_freq.resize(1024);
    }

    // Set the actual center frequency initially to the desired frequency.
    center_freq_actual = center_freq_desired;

    // Compute the divider values for the lowest and highest WSPR tones.
    double div_lo = bit_truncate(plld_actual_freq / (center_freq_desired - 1.5 * tone_spacing), -12) + std::pow(2.0, -12);
    double div_hi = bit_truncate(plld_actual_freq / (center_freq_desired + 1.5 * tone_spacing), -12);

    // If the integer portion of dividers differ, adjust the center frequency.
    if (std::floor(div_lo) != std::floor(div_hi))
    {
        center_freq_actual = plld_actual_freq / std::floor(div_lo) - 1.6 * tone_spacing;
        std::stringstream temp;
        temp << std::fixed << std::setprecision(6)
             << "Warning: center frequency has been changed to "
             << center_freq_actual / 1e6 << " MHz";
        std::cerr << temp.str() << " because of hardware limitations." << std::endl;
    }

    // Initialize tuning word table.
    double tone0_freq = center_freq_actual - 1.5 * tone_spacing;
    std::vector<long int> tuning_word(1024);

    // Generate tuning words for WSPR tones.
    for (int i = 0; i < 8; i++)
    {
        double tone_freq = tone0_freq + (i >> 1) * tone_spacing;
        double div = bit_truncate(plld_actual_freq / tone_freq, -12);

        // Apply rounding for even indices.
        if (i % 2 == 0)
        {
            div += std::pow(2.0, -12);
        }

        tuning_word[i] = static_cast<int>(div * std::pow(2.0, 12));
    }

    // Fill the remaining table with default values.
    for (int i = 8; i < 1024; i++)
    {
        double div = 500 + i;
        tuning_word[i] = static_cast<int>(div * std::pow(2.0, 12));
    }

    // Program the DMA table.
    dma_table_freq.resize(1024);
    for (int i = 0; i < 1024; i++)
    {
        dma_table_freq[i] = plld_actual_freq / (tuning_word[i] / std::pow(2.0, 12));

        // Store values in the memory-mapped page.
        reinterpret_cast<int *>(constPage.v)[i] = (0x5A << 24) + tuning_word[i];

        // Ensure adjacent tuning words have the same integer portion for valid tone generation.
        if ((i % 2 == 0) && (i < 8))
        {
            assert((tuning_word[i] & (~0xFFF)) == (tuning_word[i + 1] & (~0xFFF)));
        }
    }
}

void setup_dma_instructions_table(
    struct PageInfo &constPage,
    struct PageInfo &instrPage,
    struct PageInfo instrs[])
{
    // Allocate memory pool for DMA operation
    allocate_memory_pool(1025);

    // Allocate a memory page for storing constants
    get_real_mem_pages_from_pool(&constPage.v, &constPage.b);

    // Initialize instruction counter
    int instrCnt = 0;

    // Allocate memory pages and create DMA instructions
    while (instrCnt < 1024)
    {
        // Allocate a memory page for instructions
        get_real_mem_pages_from_pool(&instrPage.v, &instrPage.b);

        // Create DMA control blocks (CBs)
        struct CB *instr0 = reinterpret_cast<struct CB *>(instrPage.v);

        for (int i = 0; i < static_cast<int>(PAGE_SIZE / sizeof(struct CB)); i++)
        {
            // Assign virtual and bus addresses for each instruction
            instrs[instrCnt].v = reinterpret_cast<void *>(reinterpret_cast<long int>(instrPage.v) + sizeof(struct CB) * i);
            instrs[instrCnt].b = reinterpret_cast<void *>(reinterpret_cast<long int>(instrPage.b) + sizeof(struct CB) * i);

            // Configure DMA transfer: Source = constant memory page, Destination = PWM FIFO
            instr0->SOURCE_AD = reinterpret_cast<unsigned long int>(constPage.b) + 2048;
            instr0->DEST_AD = PWM_BUS_BASE + 0x18; // FIFO1
            instr0->TXFR_LEN = 4;
            instr0->STRIDE = 0;
            instr0->TI = (1 << 6) | (5 << 16) | (1 << 26); // DREQ = 1, PWM = 5, No wide mode
            instr0->RES1 = 0;
            instr0->RES2 = 0;

            // Odd instructions modify the GP0 clock divider instead of PWM FIFO
            if (i % 2)
            {
                instr0->DEST_AD = CM_GP0DIV_BUS;
                instr0->STRIDE = 4;
                instr0->TI = (1 << 26); // No wide mode
            }

            // Link previous instruction to the next in the DMA sequence
            if (instrCnt != 0)
            {
                reinterpret_cast<struct CB *>(instrs[instrCnt - 1].v)->NEXTCONBK = reinterpret_cast<long int>(instrs[instrCnt].b);
            }

            instr0++;
            instrCnt++;
        }
    }

    // Create a circular linked list of DMA instructions
    reinterpret_cast<struct CB *>(instrs[1023].v)->NEXTCONBK = reinterpret_cast<long int>(instrs[0].b);

    // Configure the PWM clock (disable, set divisor, enable)
    ACCESS_BUS_ADDR(CLK_BUS_BASE + 40 * 4) = 0x5A000026; // Source = PLLD, disable
    usleep(1000);
    ACCESS_BUS_ADDR(CLK_BUS_BASE + 41 * 4) = 0x5A002000; // Set PWM divider to 2 (250MHz)
    ACCESS_BUS_ADDR(CLK_BUS_BASE + 40 * 4) = 0x5A000016; // Source = PLLD, enable
    usleep(1000);

    // Configure PWM registers
    ACCESS_BUS_ADDR(PWM_BUS_BASE + 0x0) = 0; // Disable PWM
    usleep(1000);
    ACCESS_BUS_ADDR(PWM_BUS_BASE + 0x4) = -1; // Clear status errors
    usleep(1000);
    ACCESS_BUS_ADDR(PWM_BUS_BASE + 0x10) = 32; // Set default range
    ACCESS_BUS_ADDR(PWM_BUS_BASE + 0x20) = 32;
    ACCESS_BUS_ADDR(PWM_BUS_BASE + 0x0) = -1; // Enable FIFO mode, repeat, serializer, and channel
    usleep(1000);
    ACCESS_BUS_ADDR(PWM_BUS_BASE + 0x8) = (1 << 31) | 0x0707; // Enable DMA

    // Activate DMA
    // Obtain the base address as an integer pointer
    volatile int *dma_base = reinterpret_cast<volatile int *>((uintptr_t)dmaConfig.peri_base_virt + DMA_BUS_BASE - 0x7e000000);
    // Now cast to DMAregs pointer
    volatile struct DMAregs *DMA0 = reinterpret_cast<volatile struct DMAregs *>(dma_base);
    DMA0->CS = 1 << 31; // Reset DMA
    DMA0->CONBLK_AD = 0;
    DMA0->TI = 0;
    DMA0->CONBLK_AD = reinterpret_cast<unsigned long int>(instrPage.b);
    DMA0->CS = (1 << 0) | (255 << 16); // Enable DMA, priority level 255
}

int compute_timevalue_difference(struct timeval *result, const struct timeval *t2, const struct timeval *t1)
{
    // Compute the time difference in microseconds
    long int diff_usec = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);

    // Compute seconds and microseconds for the result
    result->tv_sec = diff_usec / 1000000;
    result->tv_usec = diff_usec % 1000000;

    // Return 1 if t2 < t1 (negative difference), otherwise 0
    return (diff_usec < 0) ? 1 : 0;
}

std::string create_time_value(const struct timeval *tv)
{
    if (!tv)
    {
        return "Invalid timeval";
    }

    char buffer[30];
    time_t curtime = tv->tv_sec;
    struct tm *timeinfo = gmtime(&curtime);

    if (!timeinfo)
    {
        return "Invalid time";
    }

    // Format the timestamp
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %T", timeinfo);

    // Construct the final string with millisecond precision
    std::ostringstream oss;
    oss << buffer << "." << ((tv->tv_usec + 500) / 1000) << " UTC";
    return oss.str();
}

void open_mailbox()
{
    // Attempt to open the mailbox
    mbox.handle = mbox_open();

    // Check for failure and handle the error
    if (mbox.handle < 0)
    {
        std::cerr << "Error: Failed to open mailbox." << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void setup_peripheral_base_virtual()
{
    // File descriptor for accessing memory
    int mem_fd;

    // Retrieve the peripheral base address
    unsigned gpio_base = get_peripheral_address();

    // Open `/dev/mem` with read/write and synchronous access
    if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
    {
        std::cerr << "Error: Cannot open /dev/mem." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Map the peripheral base address into virtual memory
    dmaConfig.peri_base_virt = static_cast<unsigned *>(mmap(
        nullptr,
        0x01000000,             // Length of mapped memory region (16MB)
        PROT_READ | PROT_WRITE, // Enable read and write access
        MAP_SHARED,             // Shared memory mapping
        mem_fd,
        gpio_base // Peripheral base physical address
        ));

    // Check for mmap failure
    if (reinterpret_cast<long int>(dmaConfig.peri_base_virt) == -1)
    {
        std::cerr << "Error: dmaConfig.peri_base_virt mmap failed." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Close the file descriptor as it is no longer needed after mmap
    close(mem_fd);
}

void remove_file(const char *filename)
{
    if (!filename)
    {
        std::cerr << "Warning: Null filename provided to remove_file()." << std::endl;
        return;
    }

    struct stat buffer;

    // Check if the file exists before attempting to remove it
    if (stat(filename, &buffer) == 0)
    {
        // Attempt to remove the file
        if (unlink(filename) != 0)
        {
            std::cerr << "Warning: Failed to remove " << filename << std::endl;
        }
    }
}

void cleanup_dma()
{
    static bool cleanup_done = false;
    if (cleanup_done)
        return; // Prevent duplicate cleanup
    cleanup_done = true;

    // Disable the clock to prevent unintended operation
    disable_clock();

    // Reset and clean up DMA-related settings
    reset_dma();

    // Deallocate memory pool to free allocated resources
    deallocate_memory_pool();

    // Remove the local device file
    remove_file(LOCAL_DEVICE_FILE_NAME);
}

void setup_dma()
{
    // Retrieve PLLD frequency
    get_plld_frequency();

    // Initialize random number generator for transmission timing
    srand(time(nullptr));

    // Initialize peripheral memory mapping
    setup_peripheral_base_virtual();

    // Set up DMA
    open_mailbox();                                             // Open mailbox for communication
    start_transmission();                                       // Enable transmission mode
    setup_dma_instructions_table(constPage, instrPage, instrs); // Configure DMA for transmission
    stop_transmission();                                        // Disable transmission mode
}

void transmit_tone(double test_tone_frequency, double ppm)
{
    // Define WSPR symbol time and tone spacing
    const double wspr_symtime = WSPR_SYMTIME;
    const double tone_spacing = 1.0 / wspr_symtime;

    // Display test tone transmission details
    std::stringstream temp;
    temp << std::setprecision(6) << std::fixed
         << "Transmitting test tone on frequency " << test_tone_frequency / 1.0e6 << " MHz.";
    std::cout << temp.str() << std::endl;
    std::cout << "Press CTRL-C to exit." << std::endl;

    // Enable transmission mode
    start_transmission();

    // DMA buffer pointer
    int bufPtr = 0;

    // DMA frequency table
    std::vector<double> dma_table_freq;

    double center_freq_actual;

    // Continuous transmission loop
    while (true)
    {
        // Validate PLLD clock frequency
        if (dmaConfig.plld_clock_frequency <= 0)
        {
            std::cerr << "Error: Invalid PLL clock frequency. Using default 500MHz." << std::endl;
            dmaConfig.plld_clock_frequency = 500000000.0;
        }

        // Compute actual PLL-adjusted frequency
        double adjusted_plld_freq = dmaConfig.plld_clock_frequency * (1 - ppm / 1e6);

        // Setup the DMA frequency table
        setup_dma_frequency_table(test_tone_frequency + 1.5 * tone_spacing, tone_spacing, adjusted_plld_freq,
                                  dma_table_freq, center_freq_actual, constPage);

        // Debug output for DMA frequency table
        std::cout << std::setprecision(30)
                  << "DEBUG: dma_table_freq[0] = " << dma_table_freq[0] << "\n"
                  << "DEBUG: dma_table_freq[1] = " << dma_table_freq[1] << "\n"
                  << "DEBUG: dma_table_freq[2] = " << dma_table_freq[2] << "\n"
                  << "DEBUG: dma_table_freq[3] = " << dma_table_freq[3] << std::endl;

        // Warn if the actual transmission frequency differs from the desired frequency
        if (center_freq_actual != test_tone_frequency + 1.5 * tone_spacing)
        {
            std::stringstream temp;
            temp << std::setprecision(6) << std::fixed
                 << "Warning: Test tone will be transmitted on "
                 << (center_freq_actual - 1.5 * tone_spacing) / 1e6
                 << " MHz due to hardware limitations.";
            std::cerr << temp.str() << std::endl;
        }

        // Transmit the test tone symbol
        transmit_symbol(0, center_freq_actual, tone_spacing, 60, dma_table_freq, F_PWM_CLK_INIT, instrs, constPage, bufPtr);
    }
}

void transmit_wspr(WsprMessage message, double wspr_frequency, double ppm, bool use_offset)
{
    // Determine if using WSPR-15 mode (longer symbol time)
    bool wspr15 =
        (wspr_frequency > 137600 && wspr_frequency < 137625) ||
        (wspr_frequency > 475800 && wspr_frequency < 475825) ||
        (wspr_frequency > 1838200 && wspr_frequency < 1838225);

    double wspr_symtime = (wspr15) ? 8.0 * WSPR_SYMTIME : WSPR_SYMTIME;
    double tone_spacing = 1.0 / wspr_symtime;

    // Apply random offset if enabled
    if ((wspr_frequency != 0) && use_offset)
    {
        wspr_frequency += (2.0 * rand() / (static_cast<double>(RAND_MAX) + 1.0) - 1.0) *
                          (wspr15 ? WSPR15_RAND_OFFSET : WSPR_RAND_OFFSET);
    }

    // Display transmission information
    std::stringstream temp;
    temp << std::setprecision(6) << std::fixed
         << "Desired center frequency for " << (wspr15 ? "WSPR-15" : "WSPR")
         << " transmission: " << wspr_frequency / 1e6 << " MHz.";
    std::cout << temp.str() << std::endl;

    // Create DMA table for transmission
    std::vector<double> dma_table_freq(1024, 0.0); // Pre-allocate for efficiency

    if (dmaConfig.plld_clock_frequency <= 0)
    {
        std::cerr << "Error: Invalid PLL clock frequency. Using default 500MHz." << std::endl;
        dmaConfig.plld_clock_frequency = 500000000.0;
    }

    setup_dma_frequency_table(wspr_frequency, tone_spacing,
                              dmaConfig.plld_clock_frequency * (1 - ppm / 1e6),
                              dma_table_freq, wspr_frequency, constPage);

    struct timeval tvBegin, tvEnd, tvDiff;
    gettimeofday(&tvBegin, NULL);

    std::cout << "TX started at: " << create_time_value(&tvBegin) << std::endl;

    struct timeval sym_start, diff;
    int bufPtr = 0;

    // Enable transmission
    start_transmission();

    // Transmit each symbol in the WSPR message
    for (int i = 0; i < MSG_SIZE; i++)
    {
        gettimeofday(&sym_start, NULL);
        compute_timevalue_difference(&diff, &sym_start, &tvBegin);
        double elapsed = diff.tv_sec + diff.tv_usec / 1e6;
        double sched_end = (i + 1) * wspr_symtime;
        double this_sym = sched_end - elapsed;
        this_sym = std::clamp(this_sym, 0.2, 2 * wspr_symtime);

        transmit_symbol(static_cast<int>(message.symbols[i]), wspr_frequency, tone_spacing,
                        this_sym, dma_table_freq, F_PWM_CLK_INIT, instrs, constPage, bufPtr);
    }

    // Disable transmission
    stop_transmission();

    // Print transmission timestamp and duration on one line
    gettimeofday(&tvEnd, nullptr);
    compute_timevalue_difference(&tvDiff, &tvEnd, &tvBegin);
    std::cout << "TX ended at: " << create_time_value(&tvEnd)
              << " (" << tvDiff.tv_sec << "." << std::setfill('0') << std::setw(3)
              << (tvDiff.tv_usec + 500) / 1000 << " s)" << std::endl;
}

// TODO:
// atexit(dma_cleanup);
// setup_dma();
// Then:
// transmit_tone(double test_tone_frequency, double ppm)
// or
// transmit_wspr(WsprMessage message, double wspr_frequency, double ppm, bool use_offset);

// TX ended at: 2025-03-28 23:25:51.1000 UTC (110.630 s)
// free(): double free detected in tcache 2
// Caught signal: 6 (Aborted).
