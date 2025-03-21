#include "dma.hpp"

// Submodules
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
// The perhipherals in the Broadcom documentation are described using their bus
// addresses and structures are created and calculations performed in this
// program to figure out how to access them with virtual addresses.

// Empirical value for F_PWM_CLK that produces WSPR symbols that are 'close' to
// 0.682s long. For some reason, despite the use of DMA, the load on the PI
// affects the TX length of the symbols. However, the varying symbol length is
// compensated for in the main loop.
#define F_PWM_CLK_INIT (31156186.6125761)

// WSRP nominal symbol time
#define WSPR_SYMTIME (8192.0 / 12000.0)
// How much random frequency offset should be added to WSPR transmissions
// if the --offset option has been turned on.
#define WSPR_RAND_OFFSET 80
#define WSPR15_RAND_OFFSET 8

// TODO:
// Not sure what the difference between a page and a block is in this
// context.  Previously these were not used so I searched for "4096"
// and guessed which one was which.
// Only the mbox.mem_ref = mem_alloc() used both valies.
#define PAGE_SIZE (4 * 1024)
#define BLOCK_SIZE (4 * 1024)

#define PWM_CLOCKS_PER_ITER_NOMINAL 1000

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

// The following are all bus addresses.
#define GPIO_BUS_BASE (0x7E200000)
#define CM_GP0CTL_BUS (0x7e101070)
#define CM_GP0DIV_BUS (0x7e101074)
#define PADS_GPIO_0_27_BUS (0x7e10002c)
#define CLK_BUS_BASE (0x7E101000)
#define DMA_BUS_BASE (0x7E007000)
#define PWM_BUS_BASE (0x7e20C000) /* PWM controller */

// Convert from a bus address to a physical address.
#define BUS_TO_PHYS(x) ((x) & ~0xC0000000)

PPMManager ppmManager;

// peri_base_virt is the base virtual address that a userspace program (this
// program) can use to read/write to the the physical addresses controlling
// the peripherals. This address is mapped at runtime using mmap and /dev/mem.
// This must be declared global so that it can be called by the atexit
// function.
volatile unsigned *peri_base_virt = NULL;

typedef enum
{
    WSPR,
    TONE
} mode_type;

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

struct wConfig
{
    // Global configuration items from command line and ini file
    bool useini = false;
    std::string inifile = "";                  // Default to empty, meaning no INI file specified
    bool xmit_enabled = true;                  // Transmission disabled by default
    bool repeat = true;                        // No repeat transmission by default
    std::string callsign = "AA0NT";            // Default to empty, requiring user input
    std::string locator = "EM18";              // Default to empty, requiring user input
    int tx_power = 20;                         // Default to 37 dBm (5W), a common WSPR power level
    std::string frequency_string = "7040100."; // Default to empty
    std::vector<double> center_freq_set = {};  // Empty vector, frequencies to be defined
    double ppm = 0.0;                          // Default to zero, meaning no frequency correction applied
    bool self_cal = true;                      // Self-calibration enabled by default
    bool random_offset = true;                 // No random offset by default
    double test_tone = 7040100;                // Default to NAN, meaning no test tone
    bool no_delay = false;                     // Delay enabled by default
    mode_type mode = WSPR;                     // Default mode is WSPR
    int terminate = -1;                        // -1 to indicate no termination signal
    bool useled = false;                       // No LED signaling by default
    bool daemon_mode = false;                  // Not running as a daemon by default
    double f_plld_clk = 125e6;                 // Default PLLD clock frequency: 125 MHz
    int mem_flag = 0;                          // Default memory flag set to 0
} config;

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

/**
 * @brief Reads a 32-bit value from the device tree ranges.
 * @details Reads an unsigned 32-bit integer from a specific offset
 *          within a device tree file.
 *
 * @param filename Path to the device tree file.
 * @param offset Byte offset in the file to read from.
 * @return The read value wrapped in std::optional, or std::nullopt on failure.
 */
std::optional<unsigned> get_dt_ranges(const std::string &filename, unsigned offset)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        return std::nullopt; // File could not be opened
    }

    file.seekg(offset);
    if (!file.good())
    {
        return std::nullopt; // Invalid seek position
    }

    unsigned char buf[4] = {}; // Buffer to store read bytes
    file.read(reinterpret_cast<char *>(buf), sizeof(buf));
    if (file.gcount() != sizeof(buf))
    {
        return std::nullopt; // Read operation failed
    }

    // Convert the 4-byte buffer to an unsigned integer (big-endian format)
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

/**
 * @brief Retrieves the base address of the Raspberry Pi peripherals.
 * @details Reads from the `/proc/device-tree/soc/ranges` file to determine
 *          the peripheral base address.
 *
 * @return The peripheral base address. Defaults to `0x20000000` if not found.
 */
unsigned get_peripheral_address()
{
    if (auto addr = get_dt_ranges("/proc/device-tree/soc/ranges", 4); addr && *addr != 0)
    {
        return *addr;
    }
    if (auto addr = get_dt_ranges("/proc/device-tree/soc/ranges", 8); addr)
    {
        return *addr;
    }

    return 0x20000000; // Default address fallback
}

/**
 * @brief Reads a formatted string value from a file.
 * @details Reads lines from a file and attempts to extract an unsigned integer
 *          based on the given format string.
 *
 * @param filename Path to the file to read.
 * @param format The format string expected in the file.
 * @return The extracted value wrapped in std::optional, or std::nullopt on failure.
 */
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

/**
 * @brief Retrieves the Raspberry Pi board revision code.
 * @details Reads `/proc/cpuinfo` and extracts the board revision.
 *
 * @return The revision code, or 0 if retrieval fails.
 */
unsigned get_revision_code()
{
    static std::optional<unsigned> cached_revision;
    if (!cached_revision)
    {
        cached_revision = read_string_from_file("/proc/cpuinfo", "Revision : %x").value_or(0);
    }
    return *cached_revision;
}

/**
 * @brief Determines the processor ID from the revision code.
 * @details Uses the revision code to identify the specific Broadcom processor.
 *
 * @return The processor ID (BCM2835, BCM2836, BCM2837, or BCM2711).
 */
int get_processor_id()
{
    unsigned rev = get_revision_code();
    return (rev & 0x800000) ? ((rev & 0xf000) >> 12) : BCM_HOST_PROCESSOR_BCM2835;
}

/**
 * @brief Configures PLLD settings based on processor type.
 * @details Determines the correct PLLD frequency and memory flag values
 *          for different Raspberry Pi versions.
 */
void getPLLD()
{
    int processor_id = get_processor_id(); // Store processor ID to avoid redundant calls

    switch (processor_id)
    {
    case BCM_HOST_PROCESSOR_BCM2835: // Raspberry Pi 1
        config.mem_flag = 0x0c;
        config.f_plld_clk = 500000000.0 * (1 - 2.500e-6); // Apply 2.5 PPM correction
        break;
    case BCM_HOST_PROCESSOR_BCM2836: // Raspberry Pi 2
    case BCM_HOST_PROCESSOR_BCM2837: // Raspberry Pi 3
        config.mem_flag = 0x04;
        config.f_plld_clk = 500000000.0; // Standard 500 MHz clock
        break;
    case BCM_HOST_PROCESSOR_BCM2711: // Raspberry Pi 4
        config.mem_flag = 0x04;
        config.f_plld_clk = 750000000.0; // Higher 750 MHz clock
        break;
    default:
        std::cerr << "Error: Unknown chipset (" << processor_id << ")." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Ensure a valid PLLD frequency
    if (config.f_plld_clk <= 0)
    {
        std::cerr << "Error: Invalid PLL clock frequency. Using default 500 MHz." << std::endl;
        config.f_plld_clk = 500000000.0;
    }
}

/**
 * @brief Allocates a memory pool for DMA usage.
 * @details Uses the mailbox interface to allocate a contiguous block of memory.
 *          The memory is locked down, mapped to virtual user space, and its
 *          bus address is stored for further use.
 *
 * @param numpages Number of memory pages to allocate.
 */
void allocMemPool(unsigned numpages)
{
    // Allocate a contiguous block of memory using the mailbox interface.
    mbox.mem_ref = mem_alloc(mbox.handle, PAGE_SIZE * numpages, BLOCK_SIZE, config.mem_flag);

    // Lock the allocated memory block and retrieve its bus address.
    mbox.bus_addr = mem_lock(mbox.handle, mbox.mem_ref);

    // Convert the bus address to a physical address and map it to user space.
    mbox.virt_addr = static_cast<unsigned char *>(mapmem(BUS_TO_PHYS(mbox.bus_addr), PAGE_SIZE * numpages));

    // Store the total number of pages allocated in the pool (constant for its lifetime).
    mbox.pool_size = numpages;

    // Initialize the count of used pages in the pool.
    mbox.pool_cnt = 0;

    // Debug print: Displays memory allocation details in hexadecimal format.
    std::cout << "DEBUG: allocMemPool bus_addr=0x"
              << std::hex << mbox.bus_addr
              << " virt_addr=0x" << reinterpret_cast<unsigned long>(mbox.virt_addr)
              << " mem_ref=0x" << mbox.mem_ref
              << std::dec << std::endl;
}

/**
 * @brief Retrieves the next available memory page from the allocated pool.
 * @details Provides a virtual and bus address for a memory page in the pool.
 *          If no more pages are available, the function prints an error and exits.
 *
 * @param[out] vAddr Pointer to store the virtual address of the allocated page.
 * @param[out] bAddr Pointer to store the bus address of the allocated page.
 */
void getRealMemPageFromPool(void **vAddr, void **bAddr)
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
    std::cout << "DEBUG: getRealMemPageFromPool bus_addr=0x"
              << std::hex << reinterpret_cast<uintptr_t>(*bAddr)
              << " virt_addr=0x" << reinterpret_cast<uintptr_t>(*vAddr)
              << std::dec << std::endl;

    // Increment the count of allocated pages.
    mbox.pool_cnt++;
}

/**
 * @brief Deallocates the memory pool.
 * @details Releases the allocated memory by unmapping virtual memory,
 *          unlocking, and freeing the memory via the mailbox interface.
 */
void deallocMemPool()
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

/**
 * @brief Disables the PWM clock.
 * @details Clears the enable bit in the clock control register and waits
 *          until the clock is no longer busy. Ensures proper synchronization.
 */
void disable_clock()
{
    // Ensure memory-mapped peripherals are initialized before proceeding.
    if (peri_base_virt == nullptr)
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

/**
 * @brief Enables TX by configuring GPIO4 and setting the clock source.
 * @details Configures GPIO4 to use alternate function 0 (GPCLK0), sets the drive
 *          strength, disables any active clock, and then enables the clock with PLLD.
 *
 * @param led (Optional) Unused parameter; defaults to false.
 */
void txon(bool led = false)
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

/**
 * @brief Disables the transmitter.
 * @details Turns off the transmission by disabling the clock source.
 */
void txoff()
{
    // Disable the clock, effectively turning off transmission.
    disable_clock();
}

/**
 * @brief Transmits a symbol for a specified duration using DMA.
 * @details Configures the DMA to transmit a symbol (tone) for the specified
 *          time interval (`tsym`). Uses PWM clocks and adjusts frequency
 *          dynamically based on the desired tone spacing.
 *
 * @param[in] sym_num The symbol number to transmit.
 * @param[in] center_freq The center frequency in Hz.
 * @param[in] tone_spacing The frequency spacing between tones in Hz.
 * @param[in] tsym The duration (seconds) for which the symbol is transmitted.
 * @param[in] dma_table_freq The DMA frequency lookup table.
 * @param[in] f_pwm_clk The frequency of the PWM clock in Hz.
 * @param[in,out] instrs The DMA instruction page array.
 * @param[in] constPage The memory page containing constant data.
 * @param[in,out] bufPtr The buffer pointer index for DMA instruction handling.
 */
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

/**
 * @brief Disables and resets the DMA engine.
 * @details Ensures that the DMA controller is properly reset before exiting.
 *          If the peripheral memory mapping is not set up, the function returns early.
 */
void unSetupDMA()
{
    // Ensure memory-mapped peripherals are initialized before proceeding.
    if (peri_base_virt == nullptr)
    {
        return;
    }

    // Obtain a pointer to the DMA control registers.
    volatile DMAregs *DMA0 = reinterpret_cast<volatile DMAregs *>(&(ACCESS_BUS_ADDR(DMA_BUS_BASE)));

    // Reset the DMA controller by setting the reset bit (bit 31) in the control/status register.
    DMA0->CS = 1 << 31;

    // Turn off transmission.
    txoff();
}

/**
 * @brief Truncates a floating-point number at a specified bit position.
 * @details Sets all bits less significant than the given LSB to zero.
 *
 * @param d The input floating-point number to be truncated.
 * @param lsb The least significant bit position to retain.
 * @return The truncated value with lower bits set to zero.
 */
double bit_trunc(const double &d, const int &lsb)
{
    // Compute the truncation factor as a power of 2.
    const double factor = std::pow(2.0, lsb);

    // Truncate the number by dividing, flooring, and multiplying back.
    return std::floor(d / factor) * factor;
}

/**
 * @brief Configures the DMA frequency table for signal generation.
 * @details Generates a tuning word table based on the desired center frequency
 *          and tone spacing, adjusting for hardware limitations if necessary.
 *
 * @param[in] center_freq_desired The desired center frequency in Hz.
 * @param[in] tone_spacing The spacing between frequency tones in Hz.
 * @param[in] plld_actual_freq The actual PLLD clock frequency in Hz.
 * @param[out] dma_table_freq The generated DMA table containing output frequencies.
 * @param[out] center_freq_actual The actual center frequency, which may be adjusted.
 * @param[in,out] constPage The PageInfo structure for storing tuning words.
 */
void setupDMATab(
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
    double div_lo = bit_trunc(plld_actual_freq / (center_freq_desired - 1.5 * tone_spacing), -12) + std::pow(2.0, -12);
    double div_hi = bit_trunc(plld_actual_freq / (center_freq_desired + 1.5 * tone_spacing), -12);

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
        double div = bit_trunc(plld_actual_freq / tone_freq, -12);

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

/**
 * @brief Configures and initializes DMA for PWM signal generation.
 * @details Allocates memory pages, creates DMA control blocks, sets up a circular
 *          linked list of DMA instructions, and configures the PWM clock and registers.
 *
 * @param[out] constPage PageInfo structure for storing constant data.
 * @param[out] instrPage PageInfo structure for the initial DMA instruction page.
 * @param[out] instrs Array of PageInfo structures for DMA instructions.
 */
void setupDMA(
    struct PageInfo &constPage,
    struct PageInfo &instrPage,
    struct PageInfo instrs[])
{
    // Allocate memory pool for DMA operation
    allocMemPool(1025);

    // Allocate a memory page for storing constants
    getRealMemPageFromPool(&constPage.v, &constPage.b);

    // Initialize instruction counter
    int instrCnt = 0;

    // Allocate memory pages and create DMA instructions
    while (instrCnt < 1024)
    {
        // Allocate a memory page for instructions
        getRealMemPageFromPool(&instrPage.v, &instrPage.b);

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
    volatile int *dma_base = reinterpret_cast<volatile int *>((uintptr_t)peri_base_virt + DMA_BUS_BASE - 0x7e000000);
    // Now cast to DMAregs pointer
    volatile struct DMAregs *DMA0 = reinterpret_cast<volatile struct DMAregs *>(dma_base);
    DMA0->CS = 1 << 31; // Reset DMA
    DMA0->CONBLK_AD = 0;
    DMA0->TI = 0;
    DMA0->CONBLK_AD = reinterpret_cast<unsigned long int>(instrPage.b);
    DMA0->CS = (1 << 0) | (255 << 16); // Enable DMA, priority level 255
}

/**
 * @brief Waits until the system time reaches one second past a specified minute interval.
 * @details Continuously checks the system clock and waits until `tm_min % minute == 0`
 *          and `tm_sec == 0`. Once the condition is met, it waits an additional second
 *          before returning.
 *
 * @param[in] minute The minute interval at which to synchronize execution.
 * @return `true` when the time condition is met, allowing the program to proceed.
 */
bool wait_every(int minute)
{
    time_t t;
    struct tm *ptm;

    // Loop until the system time reaches the desired minute and second
    while (true)
    {
        time(&t);
        ptm = gmtime(&t);

        // Check if the current minute matches the interval and the second is 0
        if ((ptm->tm_min % minute) == 0 && ptm->tm_sec == 0)
        {
            break;
        }

        // Sleep for 1 millisecond to avoid excessive CPU usage
        usleep(1000);
    }

    // Ensure at least one full second has passed before proceeding
    usleep(1000000);

    return true; // OK to proceed
}

/**
 * @brief Computes the difference between two time values.
 * @details Calculates `t2 - t1` and stores the result in `result`. If `t2 < t1`,
 *          the function returns `1`, otherwise, it returns `0`.
 *
 * @param[out] result Pointer to `timeval` struct to store the difference.
 * @param[in] t2 Pointer to the later `timeval` structure.
 * @param[in] t1 Pointer to the earlier `timeval` structure.
 * @return Returns `1` if the difference is negative (t2 < t1), otherwise `0`.
 */
int timeval_subtract(struct timeval *result, const struct timeval *t2, const struct timeval *t1)
{
    // Compute the time difference in microseconds
    long int diff_usec = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);

    // Compute seconds and microseconds for the result
    result->tv_sec = diff_usec / 1000000;
    result->tv_usec = diff_usec % 1000000;

    // Return 1 if t2 < t1 (negative difference), otherwise 0
    return (diff_usec < 0) ? 1 : 0;
}

/**
 * @brief Formats a timestamp from a `timeval` structure into a string.
 * @details Converts the given `timeval` structure into a formatted `YYYY-MM-DD HH:MM:SS.mmm UTC` string.
 *
 * @param[in] tv Pointer to `timeval` structure containing the timestamp.
 * @return A formatted string representation of the timestamp.
 */
std::string timeval_print(const struct timeval *tv)
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

/**
 * @brief Opens the mailbox device.
 * @details Creates the mailbox special files and attempts to open the mailbox.
 *          If opening the mailbox fails, an error message is printed, and the program exits.
 */
void open_mbox()
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

/**
 * @brief Sets the scheduling priority of the current thread.
 * @details Assigns the thread to the `SCHED_FIFO` real-time scheduling policy with
 *          the specified priority. Requires superuser privileges.
 *
 * @param[in] priority The priority level to set (higher values indicate higher priority).
 */
void setSchedPriority(int priority)
{
    // Define scheduling parameters
    struct sched_param sp;
    sp.sched_priority = priority;

    // Attempt to set thread scheduling parameters to SCHED_FIFO with the given priority
    int ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);

    // Handle potential failure
    if (ret != 0)
    {
        std::cerr << "Warning: pthread_setschedparam (increase thread priority) failed with error code: "
                  << ret << std::endl;
    }
}

/**
 * @brief Maps peripheral base address to virtual memory.
 * @details Creates a memory-mapped region for accessing the peripheral range of physical memory.
 *          This function opens `/dev/mem` and maps the peripheral base address.
 *
 * @param[out] peri_base_virt Reference to a pointer that will store the mapped virtual memory address.
 */
void setup_peri_base_virt(volatile unsigned *&peri_base_virt)
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
    peri_base_virt = static_cast<unsigned *>(mmap(
        nullptr,
        0x01000000,             // Length of mapped memory region (16MB)
        PROT_READ | PROT_WRITE, // Enable read and write access
        MAP_SHARED,             // Shared memory mapping
        mem_fd,
        gpio_base // Peripheral base physical address
        ));

    // Check for mmap failure
    if (reinterpret_cast<long int>(peri_base_virt) == -1)
    {
        std::cerr << "Error: peri_base_virt mmap failed." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Close the file descriptor as it is no longer needed after mmap
    close(mem_fd);
}

/**
 * @brief Safely removes a file if it exists.
 * @details Checks whether the specified file exists before attempting to remove it.
 *          If the file exists but removal fails, a warning is displayed.
 *
 * @param[in] filename Pointer to a null-terminated string containing the file path.
 */
void safe_remove(const char *filename)
{
    if (!filename)
    {
        std::cerr << "Warning: Null filename provided to safe_remove()." << std::endl;
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

/**
 * @brief Cleans up DMA resources.
 * @details Disables the clock, resets the DMA engine, deallocates memory,
 *          and removes the local device file if it exists.
 */
void dma_cleanup()
{
    static bool cleanup_done = false;
    if (cleanup_done) return; // Prevent duplicate cleanup
    cleanup_done = true;

    // Disable the clock to prevent unintended operation
    disable_clock();

    // Reset and clean up DMA-related settings
    unSetupDMA();

    // Deallocate memory pool to free allocated resources
    deallocMemPool();

    // Remove the local device file
    safe_remove(LOCAL_DEVICE_FILE_NAME);
}

bool ppm_init()
{
    // Initialize PPM Manager
    PPMStatus status = ppmManager.initialize();

    // Handle initialization errors
    if (status == PPMStatus::ERROR_UNSYNCHRONIZED_TIME)
    {
        std::cerr << "System time is not synchronized." << std::endl;
        return false;
    }
    else if (status == PPMStatus::ERROR_CHRONY_NOT_FOUND)
    {
        std::cerr << "Chrony not found. Using clock drift measurement." << std::endl;
    }

    std::cout << "Current PPM: " << ppmManager.getCurrentPPM() << std::endl;

    return true;
}

/**
 * @brief Configures and initializes the DMA system for transmission.
 * @details Retrieves PLLD frequency, calibrates timing, sets scheduling priority,
 *          initializes random number generator, validates the frequency setting,
 *          and sets up DMA with peripherals.
 */
void setup_dma()
{
    // Retrieve PLLD frequency
    getPLLD();

    // Perform initial time/clock calibration
    ppm_init();
    config.ppm = ppmManager.getCurrentPPM();

    // Set high scheduling priority to reduce kernel interruptions
    setSchedPriority(30);

    // Initialize random number generator for transmission timing
    srand(time(nullptr));

    // Push a single hardcoded band to `center_freq_set`
    double temp_center_freq_desired;
    try
    {
        // Convert frequency string to double and push to the set
        temp_center_freq_desired = std::stod(config.frequency_string);
        config.center_freq_set.push_back(temp_center_freq_desired);

        // Ensure the frequency value is valid
        if (!std::isfinite(temp_center_freq_desired))
        {
            throw std::runtime_error("Invalid floating-point value");
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: Invalid frequency string. Using default 7040100 Hz." << std::endl;
        config.center_freq_set.push_back(7040100.0);
    }

    // Initialize peripheral memory mapping
    setup_peri_base_virt(peri_base_virt);

    // Set up DMA
    open_mbox();                            // Open mailbox for communication
    txon();                                 // Enable transmission mode
    setupDMA(constPage, instrPage, instrs); // Configure DMA for transmission
    txoff();                                // Disable transmission mode
}

/**
 * @brief Continuously transmits a test tone.
 * @details Generates a single test tone using the DMA setup, continuously transmitting it
 *          until the user exits with `CTRL-C`. The tone frequency is dynamically adjusted
 *          based on PPM calibration.
 */
void tx_tone()
{
    // Define WSPR symbol time and tone spacing
    const double wspr_symtime = WSPR_SYMTIME;
    const double tone_spacing = 1.0 / wspr_symtime;

    // Display test tone transmission details
    std::stringstream temp;
    temp << std::setprecision(6) << std::fixed
         << "Transmitting test tone on frequency " << config.test_tone / 1.0e6 << " MHz.";
    std::cout << temp.str() << std::endl;
    std::cout << "Press CTRL-C to exit." << std::endl;

    // Enable transmission mode
    txon();

    // DMA buffer pointer
    int bufPtr = 0;

    // DMA frequency table
    std::vector<double> dma_table_freq;

    // Set to an arbitrary non-zero value to ensure `setupDMATab` is called at least once
    double ppm_prev = 123456;
    double center_freq_actual;

    // Continuous transmission loop
    while (true)
    {
        // Perform self-calibration if enabled
        if (config.self_cal)
        {
            config.ppm = ppmManager.getCurrentPPM();
        }

        // If PPM value has changed, update the DMA table
        if (config.ppm != ppm_prev)
        {
            // Validate PLLD clock frequency
            if (config.f_plld_clk <= 0)
            {
                std::cerr << "Error: Invalid PLL clock frequency. Using default 500MHz." << std::endl;
                config.f_plld_clk = 500000000.0;
            }

            // Compute actual PLL-adjusted frequency
            double adjusted_plld_freq = config.f_plld_clk * (1 - config.ppm / 1e6);

            // Setup the DMA frequency table
            setupDMATab(config.test_tone + 1.5 * tone_spacing, tone_spacing, adjusted_plld_freq,
                        dma_table_freq, center_freq_actual, constPage);

            // Debug output for DMA frequency table
            std::cout << std::setprecision(30)
                      << "DEBUG: dma_table_freq[0] = " << dma_table_freq[0] << "\n"
                      << "DEBUG: dma_table_freq[1] = " << dma_table_freq[1] << "\n"
                      << "DEBUG: dma_table_freq[2] = " << dma_table_freq[2] << "\n"
                      << "DEBUG: dma_table_freq[3] = " << dma_table_freq[3] << std::endl;

            // Warn if the actual transmission frequency differs from the desired frequency
            if (center_freq_actual != config.test_tone + 1.5 * tone_spacing)
            {
                std::stringstream temp;
                temp << std::setprecision(6) << std::fixed
                     << "Warning: Test tone will be transmitted on "
                     << (center_freq_actual - 1.5 * tone_spacing) / 1e6
                     << " MHz due to hardware limitations.";
                std::cerr << temp.str() << std::endl;
            }

            // Update previous PPM value to avoid redundant updates
            ppm_prev = config.ppm;
        }

        // Transmit the test tone symbol
        txSym(0, center_freq_actual, tone_spacing, 60, dma_table_freq, F_PWM_CLK_INIT, instrs, constPage, bufPtr);
    }
}

/**
 * @brief Transmits WSPR (Weak Signal Propagation Reporter) messages.
 * @details Continuously loops through available bands, waiting for WSPR transmission
 *          windows, generating WSPR symbols, and transmitting them.
 *          The function dynamically adjusts transmission frequency based on
 *          PPM calibration and ensures accurate symbol timing.
 */
void tx_wspr()
{
    // Retrieve the number of bands available for transmission
    int nbands = config.center_freq_set.size();

    while (true) // Reload Loop
    {
        // Generate a new WSPR message
        WsprMessage message(config.callsign, config.locator, config.tx_power);

        // Print encoded packet
        std::cout << "WSPR codeblock: ";
        for (int i = 0; i < MSG_SIZE; i++)
        {
            if (i > 0)
            {
                std::cout << ",";
            }
            std::cout << static_cast<int>(message.symbols[i]); // Ensure correct formatting
        }
        std::cout << std::endl;

        std::cout << "Ready to transmit (setup complete)." << std::endl;

        int band = 0;
        int n_tx = 0;

        while (true)
        {
            // Validate available bands
            if (config.center_freq_set.empty())
            {
                std::cerr << "Error: center_freq_set is empty. Cannot access band [" << band << "]." << std::endl;
                exit(1);
            }
            if (band < 0 || band >= static_cast<int>(config.center_freq_set.size()))
            {
                std::cerr << "Error: Band index out of range. band [" << band << "] size [" << config.center_freq_set.size() << "]." << std::endl;
                exit(1);
            }

            // Determine center frequency
            double center_freq_desired = config.center_freq_set[band];

            // Determine if using WSPR-15 mode (longer symbol time)
            bool wspr15 =
                (center_freq_desired > 137600 && center_freq_desired < 137625) ||
                (center_freq_desired > 475800 && center_freq_desired < 475825) ||
                (center_freq_desired > 1838200 && center_freq_desired < 1838225);

            double wspr_symtime = (wspr15) ? 8.0 * WSPR_SYMTIME : WSPR_SYMTIME;
            double tone_spacing = 1.0 / wspr_symtime;

            // Apply random offset if enabled
            if ((center_freq_desired != 0) && config.random_offset)
            {
                center_freq_desired += (2.0 * rand() / (static_cast<double>(RAND_MAX) + 1.0) - 1.0) *
                                       (wspr15 ? WSPR15_RAND_OFFSET : WSPR_RAND_OFFSET);
            }

            // Display transmission information
            std::stringstream temp;
            temp << std::setprecision(6) << std::fixed
                 << "Desired center frequency for " << (wspr15 ? "WSPR-15" : "WSPR")
                 << " transmission: " << center_freq_desired / 1e6 << " MHz.";
            std::cout << temp.str() << std::endl;

            // Wait for the next WSPR transmission window
            if (config.no_delay)
            {
                std::cout << "Transmitting immediately (not waiting for WSPR window)." << std::endl;
            }
            else
            {
                std::cout << "Waiting for next WSPR transmission window." << std::endl;
                if (!wait_every((wspr15) ? 15 : 2))
                {
                    // Reload if ini changes
                    break;
                }
            }

            // Update frequency correction (PPM) if enabled
            if (config.self_cal)
            {
                config.ppm = ppmManager.getCurrentPPM();
            }

            // Create DMA table for transmission
            std::vector<double> dma_table_freq(1024, 0.0); // Pre-allocate for efficiency
            double center_freq_actual = center_freq_desired;

            if (center_freq_desired)
            {
                if (config.f_plld_clk <= 0)
                {
                    std::cerr << "Error: Invalid PLL clock frequency. Using default 500MHz." << std::endl;
                    config.f_plld_clk = 500000000.0;
                }

                setupDMATab(center_freq_desired, tone_spacing,
                            config.f_plld_clk * (1 - config.ppm / 1e6),
                            dma_table_freq, center_freq_actual, constPage);
            }

            // Start transmission if conditions are met
            if (center_freq_actual && config.xmit_enabled)
            {
                struct timeval tvBegin, tvEnd, tvDiff;
                gettimeofday(&tvBegin, NULL);

                std::cout << "TX started at: " << timeval_print(&tvBegin) << std::endl;

                struct timeval sym_start, diff;
                int bufPtr = 0;

                // Enable transmission
                txon();

                // Transmit each symbol in the WSPR message
                for (int i = 0; i < MSG_SIZE; i++)
                {
                    gettimeofday(&sym_start, NULL);
                    timeval_subtract(&diff, &sym_start, &tvBegin);
                    double elapsed = diff.tv_sec + diff.tv_usec / 1e6;
                    double sched_end = (i + 1) * wspr_symtime;
                    double this_sym = sched_end - elapsed;
                    this_sym = std::clamp(this_sym, 0.2, 2 * wspr_symtime);

                    txSym(static_cast<int>(message.symbols[i]), center_freq_actual, tone_spacing,
                          this_sym, dma_table_freq, F_PWM_CLK_INIT, instrs, constPage, bufPtr);
                }
                n_tx++;

                // Disable transmission
                txoff();

                // Print transmission timestamp and duration on one line
                gettimeofday(&tvEnd, nullptr);
                timeval_subtract(&tvDiff, &tvEnd, &tvBegin);
                std::cout << "TX ended at: " << timeval_print(&tvEnd)
                          << " (" << tvDiff.tv_sec << "." << std::setfill('0') << std::setw(3)
                          << (tvDiff.tv_usec + 500) / 1000 << " s)" << std::endl;
            }
            else
            {
                std::cout << "Skipping transmission." << std::endl;
                usleep(1000000);
            }

            // Cycle through available bands
            band = (band + 1) % nbands;
            if ((band == 0) && !config.repeat)
            {
                return;
            }
            if ((config.terminate > 0) && (n_tx >= config.terminate))
            {
                return;
            }
        }
    } // Reload Loop
}
