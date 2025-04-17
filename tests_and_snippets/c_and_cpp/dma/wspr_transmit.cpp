// TODO: Make into a class

#include "wspr_transmit.hpp" // Required: implements the functions declared here

// Submodules
#include "wspr_message.hpp" // Required: defines WsprMessage

// C++ Standard Library Headers
#include <algorithm> // Required: std::clamp
#include <cassert>   // Required: assert()
#include <cmath>     // Required: std::pow, std::floor, std::round
#include <cstdint>   // Required: uint32_t, uintptr_t
#include <cstdlib>   // Required: std::rand
#include <cstring>   // Required: std::memcpy
#include <fstream>   // Required: std::ifstream
#include <iomanip>   // Required: std::setprecision, setw, setfill
#include <iostream>  // Required: std::cout, std::cerr
#include <optional>  // Required: std::optional
#include <random>    // Required: std::random_device, mt19937, uniform_real_distribution
#include <sstream>   // Required: std::stringstream
#include <stdexcept> // Required: std::runtime_error
#include <string>    // Required: std::string
#include <vector>    // Required: std::vector

// C Standard Library Headers
#include <sys/types.h> // Required by sys/stat.h on some systems → keep
#include <unistd.h>    // Required: close(), unlink(), usleep()

// POSIX & System-Specific Headers
#include <fcntl.h>    // Required: open() flags
#include <sys/mman.h> // Required: mmap(), munmap()
#include <sys/stat.h> // Required: struct stat, stat()
#include <sys/time.h> // Required: gettimeofday(), struct timeval

#ifdef DEBUG_WSPR_TRANSMIT
constexpr const bool debug = true;
#else
constexpr const bool debug = false;
#endif

#ifdef __cplusplus
extern "C"
{
#include "mailbox.h"
}
#endif /* __cplusplus */

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

/**
 * @brief Processor ID for the Broadcom BCM2835 chip.
 *
 * This constant identifies the BCM2835 processor, which is used in the original Raspberry Pi (RPi1).
 */
constexpr int BCM_HOST_PROCESSOR_BCM2835 = 0; // BCM2835 (RPi1)

/**
 * @brief Processor ID for the Broadcom BCM2836 chip.
 *
 * This constant identifies the BCM2836 processor, which is used in the Raspberry Pi 2 (RPi2).
 */
constexpr int BCM_HOST_PROCESSOR_BCM2836 = 1; // BCM2836 (RPi2)

/**
 * @brief Processor ID for the Broadcom BCM2837 chip.
 *
 * This constant identifies the BCM2837 processor, which is used in the Raspberry Pi 3 (RPi3).
 */
constexpr int BCM_HOST_PROCESSOR_BCM2837 = 2; // BCM2837 (RPi3)

/**
 * @brief Processor ID for the Broadcom BCM2711 chip.
 *
 * This constant identifies the BCM2711 processor, which is used in the Raspberry Pi 4 (RPi4).
 */
constexpr int BCM_HOST_PROCESSOR_BCM2711 = 3; // BCM2711 (RPi4)

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
 * @brief Global configuration object.
 *
 * This DMAConfig instance holds the transmission functionality global objects.
 */
struct DMAConfig dmaConfig;

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
 * @brief Global instance of transmission parameters for WSPR.
 *
 * This global variable holds the current settings used for a WSPR transmission,
 * including the WSPR message, transmission frequency, symbol time, tone spacing,
 * and the DMA frequency lookup table.
 */
struct WsprTransmissionParams transParams;

/**
 * @brief Calculates the virtual address for a given bus address.
 *
 * This function calculates the appropriate virtual address for accessing the
 * given bus address in the peripheral space. It does so by subtracting 0x7e000000
 * from the supplied bus address to obtain the offset into the peripheral address space,
 * then adds that offset to the virtual base address of the peripherals.
 *
 * @param bus_addr The bus address from which to calculate the corresponding virtual address.
 * @return A reference to a volatile int located at the computed virtual address.
 */
inline volatile int &accessBusAddress(std::uintptr_t bus_addr)
{
    // Compute byte‐offset from the bus address
    std::uintptr_t offset = bus_addr - 0x7E000000UL;

    // Treat the void* as a char* (1‐byte pointer) so pointer arithmetic
    // is in bytes.
    auto base = static_cast<char *>(dmaConfig.peripheral_base_virtual);

    // Add the offset, then cast to a volatile‐int pointer and dereference.
    return *reinterpret_cast<volatile int *>(base + offset);
}

/**
 * @brief Sets a specified bit at a bus address in the peripheral address space.
 *
 * This function accesses the virtual address corresponding to the given bus address using
 * the `accessBusAddress()` function and sets the bit at the provided position.
 *
 * @param base The bus address in the peripheral address space.
 * @param bit The bit number to set (0-indexed).
 */
inline void setBitBusAddress(std::uintptr_t base, unsigned int bit)
{
    accessBusAddress(base) |= 1 << bit;
}

/**
 * @brief Clears a specified bit at a bus address in the peripheral address space.
 *
 * This function accesses the virtual address corresponding to the given bus address using
 * the `accessBusAddress()` function and clears the bit at the provided position.
 *
 * @param base The bus address in the peripheral address space.
 * @param bit The bit number to clear (0-indexed).
 */
inline void clearBitBusAddress(std::uintptr_t base, unsigned int bit)
{
    accessBusAddress(base) &= ~(1 << bit);
}

/**
 * @brief Converts a bus address to a physical address.
 *
 * This function converts a given bus address into the corresponding physical address by
 * masking out the upper bits (0xC0000000) that are not part of the physical address.
 *
 * @param x The bus address.
 * @return The physical address obtained from the bus address.
 */
inline std::uintptr_t busToPhys(std::uintptr_t x)
{
    return x & ~0xC0000000UL;
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
int symbol_timeval_subtract(struct timeval *result, const struct timeval *t2, const struct timeval *t1)
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
 * @brief Initialize DMAConfig PLLD frequencies and mailbox memory flag.
 *
 * @details
 *   1. Reads the Raspberry Pi hardware revision from `/proc/cpuinfo` (cached
 *      after first read).
 *   2. Determines the processor ID (BCM2835, BCM2836/37, or BCM2711).
 *   3. Sets `dmaConfig.mem_flag` to the correct mailbox allocation flag.
 *   4. Sets `dmaConfig.plld_nominal_freq` to the board’s true PLLD base
 *      frequency (500 MHz for Pi 1/2/3, 750 MHz for Pi 4).
 *   5. Initializes `dmaConfig.plld_clock_frequency` equal to
 *      `plld_nominal_freq` (zero PPM correction).
 *
 * @throws std::runtime_error if the processor ID is unrecognized.
 */
void get_plld_and_memflag()
{
    // Cache the revision to avoid repeated file I/O
    static std::optional<unsigned> cached_revision;
    if (!cached_revision)
    {
        std::ifstream file("/proc/cpuinfo");
        if (file)
        {
            std::string line;
            unsigned value = 0;
            const std::string pattern = "Revision\t: %x";
            while (std::getline(file, line))
            {
                if (sscanf(line.c_str(), pattern.c_str(), &value) == 1)
                {
                    cached_revision = value;
                    break;
                }
            }
        }
        if (!cached_revision)
        {
            cached_revision = 0; // fallback if parsing fails
        }
    }

    const unsigned rev = *cached_revision;

    // Extract processor ID (high‑bit indicates new format)
    const int proc_id = (rev & 0x800000)
                            ? ((rev & 0xF000) >> 12)
                            : BCM_HOST_PROCESSOR_BCM2835;

    // Determine base PLLD frequency and mailbox flag
    double base_freq_hz = 500e6;
    switch (proc_id)
    {
    case BCM_HOST_PROCESSOR_BCM2835: // Pi 1
        dmaConfig.mem_flag = 0x0C;
        base_freq_hz = 500e6;
        break;
    case BCM_HOST_PROCESSOR_BCM2836: // Pi 2
    case BCM_HOST_PROCESSOR_BCM2837: // Pi 3
        dmaConfig.mem_flag = 0x04;
        base_freq_hz = 500e6;
        break;
    case BCM_HOST_PROCESSOR_BCM2711: // Pi 4
        dmaConfig.mem_flag = 0x04;
        base_freq_hz = 750e6;
        break;
    default:
        throw std::runtime_error(
            "Error: Unknown chipset (" + std::to_string(proc_id) + ")");
    }

    // Store nominal and initial (zero‑PPM) working frequency
    dmaConfig.plld_nominal_freq = base_freq_hz;
    dmaConfig.plld_clock_frequency = base_freq_hz;

    // Sanity check
    if (dmaConfig.plld_clock_frequency <= 0)
    {
        std::cerr << "Error: Invalid PLLD frequency; defaulting to 500 MHz\n";
        dmaConfig.plld_nominal_freq = 500e6;
        dmaConfig.plld_clock_frequency = 500e6;
    }
}

/**
 * @brief Maps peripheral base address to virtual memory.
 *
 * Reads the Raspberry Pi's device tree to determine the peripheral base
 * address, then memory-maps that region for access via virtual memory.
 *
 * This is used for low-level register access to GPIO, clocks, DMA, etc.
 *
 * @param[out] dmaConfig.peripheral_base_virtual Reference to a pointer that will
 *             be set to the mapped virtual memory address.
 *
 * @throws Terminates the program if the peripheral base cannot be determined,
 *         `/dev/mem` cannot be opened, or `mmap` fails.
 */
void setup_peripheral_base_virtual()
{
    auto read_dt_range = [](const std::string &filename, unsigned offset) -> std::optional<unsigned>
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file)
            return std::nullopt;

        file.seekg(offset);
        if (!file.good())
            return std::nullopt;

        unsigned char buf[4] = {};
        file.read(reinterpret_cast<char *>(buf), sizeof(buf));
        if (file.gcount() != sizeof(buf))
            return std::nullopt;

        // Big‑endian to host‑endian conversion
        return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    };

    unsigned peripheral_base = 0x20000000; // Default fallback
    if (auto addr = read_dt_range("/proc/device-tree/soc/ranges", 4); addr && *addr != 0)
    {
        peripheral_base = *addr;
    }
    else if (auto addr = read_dt_range("/proc/device-tree/soc/ranges", 8); addr)
    {
        peripheral_base = *addr;
    }

    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0)
    {
        throw std::runtime_error("Error: Cannot open /dev/mem.");
    }

    // mmap returns void*, so assign directly
    dmaConfig.peripheral_base_virtual = mmap(
        nullptr,
        0x01000000,             // 16 MB
        PROT_READ | PROT_WRITE, // read/write
        MAP_SHARED,
        mem_fd,
        peripheral_base);
    close(mem_fd);

    if (!dmaConfig.peripheral_base_virtual)
        throw std::runtime_error("peripheral_base_virtual not initialized");

    // Check against MAP_FAILED, not –1
    if (dmaConfig.peripheral_base_virtual == MAP_FAILED)
    {
        throw std::runtime_error("Error: peripheral_base_virtual mmap failed.");
    }
}

/**
 * @brief Allocate a pool of DMA‑capable memory pages.
 *
 * @details Uses the Broadcom mailbox interface to:
 *   1. Allocate a contiguous block of physical pages.
 *   2. Lock the block and retrieve its bus address.
 *   3. Map the block into user space for CPU access.
 *   4. Track the pool size and reset the per‑page allocation counter.
 *
 * When called with `numpages = 1025`, one page is reserved for constant data
 * and 1024 pages are used for building the DMA instruction chain.
 *
 * @param numpages Total number of pages to allocate (1 constant + N instruction pages).
 * @throws std::runtime_error if mailbox allocation, locking, or mapping fails.
 */
void allocate_memory_pool(unsigned numpages)
{
    // Allocate a contiguous block of physical pages
    mbox.mem_ref = mem_alloc(
        mbox.handle,
        PAGE_SIZE * numpages,
        BLOCK_SIZE,
        dmaConfig.mem_flag);
    if (mbox.mem_ref == 0)
    {
        throw std::runtime_error("Error: mem_alloc failed.");
    }

    // Lock the block to obtain its bus address
    mbox.bus_addr = mem_lock(mbox.handle, mbox.mem_ref);
    if (mbox.bus_addr == 0)
    {
        mem_free(mbox.handle, mbox.mem_ref);
        throw std::runtime_error("Error: mem_lock failed.");
    }

    // Map the locked pages into user‑space virtual memory
    mbox.virt_addr = static_cast<unsigned char *>(
        mapmem(busToPhys(mbox.bus_addr), PAGE_SIZE * numpages));
    if (mbox.virt_addr == nullptr)
    {
        mem_unlock(mbox.handle, mbox.mem_ref);
        mem_free(mbox.handle, mbox.mem_ref);
        throw std::runtime_error("Error: mapmem failed.");
    }

    // Record pool parameters
    mbox.pool_size = numpages; // total pages available
    mbox.pool_cnt = 0;         // pages allocated so far

    if (debug)
    {
        // Debug output: Show allocated bus & virtual addresses
        std::cout << "DEBUG: allocate_memory_pool bus_addr=0x"
                  << std::hex << mbox.bus_addr
                  << " virt_addr=0x" << reinterpret_cast<unsigned long>(mbox.virt_addr)
                  << " mem_ref=0x" << mbox.mem_ref
                  << std::dec << std::endl;
    }
}

/**
 * @brief Retrieves the next available memory page from the allocated pool.
 * @details Provides a virtual and bus address for a memory page in the pool.
 *          If no more pages are available, the function prints an error and exits.
 *
 * @param[out] vAddr Pointer to store the virtual address of the allocated page.
 * @param[out] bAddr Pointer to store the bus address of the allocated page.
 */
void get_real_mem_page_from_pool(void **vAddr, void **bAddr)
{
    // Ensure that we do not exceed the allocated pool size.
    if (mbox.pool_cnt >= mbox.pool_size)
    {
        throw std::runtime_error("Error: unable to allocate more pages.");
    }

    // Compute the offset for the next available page.
    unsigned offset = mbox.pool_cnt * PAGE_SIZE;

    // Retrieve the virtual and bus addresses based on the offset.
    *vAddr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(mbox.virt_addr) + offset);
    *bAddr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(mbox.bus_addr) + offset);

    if (debug)
    {
        // Debug print: Displays allocated memory details.
        std::cout << "DEBUG: get_real_mem_page_from_pool bus_addr=0x"
                  << std::hex << reinterpret_cast<uintptr_t>(*bAddr)
                  << " virt_addr=0x" << reinterpret_cast<uintptr_t>(*vAddr)
                  << std::dec << std::endl;
    }

    // Increment the count of allocated pages.
    mbox.pool_cnt++;
}

/**
 * @brief Deallocates the memory pool.
 * @details Releases the allocated memory by unmapping virtual memory,
 *          unlocking, and freeing the memory via the mailbox interface.
 */
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

/**
 * @brief Disables the PWM clock.
 * @details Clears the enable bit in the clock control register and waits
 *          until the clock is no longer busy. Ensures proper synchronization.
 */
void disable_clock()
{
    // Ensure memory-mapped peripherals are initialized before proceeding.
    if (dmaConfig.peripheral_base_virtual == nullptr)
    {
        return;
    }

    // Read current clock settings from the clock control register.
    auto settings = accessBusAddress(CM_GP0CTL_BUS);

    // Disable the clock: clear the enable bit while preserving other settings.
    // Apply the required password (0x5A000000) to modify the register.
    settings = (settings & 0x7EF) | 0x5A000000;
    accessBusAddress(CM_GP0CTL_BUS) = static_cast<int>(settings);

    // Wait until the clock is no longer busy.
    while (accessBusAddress(CM_GP0CTL_BUS) & (1 << 7))
    {
        // Busy-wait loop to ensure clock disable is complete.
    }
}

/**
 * @brief Enables TX by configuring GPIO4 and setting the clock source.
 * @details Configures GPIO4 to use alternate function 0 (GPCLK0), sets the drive
 *          strength, disables any active clock, and then enables the clock with PLLD.
 */
void transmit_on()
{
    // Configure GPIO4 function select (Fsel) to alternate function 0 (GPCLK0).
    // This setting follows Section 6.2 of the ARM Peripherals Manual.
    setBitBusAddress(GPIO_BUS_BASE, 14);   // Set bit 14
    clearBitBusAddress(GPIO_BUS_BASE, 13); // Clear bit 13
    clearBitBusAddress(GPIO_BUS_BASE, 12); // Clear bit 12

    // Set GPIO drive strength, values range from 2mA (-3.4dBm) to 16mA (+10.6dBm)
    accessBusAddress(PADS_GPIO_0_27_BUS) = 0x5a000018 + transParams.power;

    // Define clock control structure and set PLLD as the clock source.
    struct GPCTL setupword = {6 /*SRC*/, 0, 0, 0, 0, 3, 0x5A};

    // Enable the clock by modifying the control word.
    setupword = {6 /*SRC*/, 1, 0, 0, 0, 3, 0x5A};
    int temp;
    std::memcpy(&temp, &setupword, sizeof(int));

    // Apply clock control settings.
    accessBusAddress(CM_GP0CTL_BUS) = temp;
}

/**
 * @brief Disables the transmitter.
 * @details Turns off the transmission by disabling the clock source.
 */
void transmit_off()
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
 * @param[in] tsym The duration (seconds) for which the symbol is transmitted.
 * @param[in,out] bufPtr The buffer pointer index for DMA instruction handling.
 */
void transmit_symbol(
    const int &sym_num,
    const double &tsym,
    int &bufPtr)
{
    // Determine indices for DMA frequency table
    const int f0_idx = sym_num * 2;
    const int f1_idx = f0_idx + 1;

    // Retrieve frequency values from the DMA table
    const double f0_freq = transParams.dma_table_freq[f0_idx];
    const double f1_freq = transParams.dma_table_freq[f1_idx];

    // Calculate the expected tone frequency
    const double tone_freq = transParams.frequency - 1.5 * transParams.tone_spacing + sym_num * transParams.tone_spacing;

    // Ensure the tone frequency is within bounds
    assert((tone_freq >= f0_freq) && (tone_freq <= f1_freq));

    // Compute frequency ratio for symbol transmission
    const double f0_ratio = 1.0 - (tone_freq - f0_freq) / (f1_freq - f0_freq);

    if (debug)
        std::cout << "DEBUG: f0_ratio = " << f0_ratio << std::endl;

    // Ensure f0_ratio is between 0.0 and 1.0
    assert((f0_ratio >= 0) && (f0_ratio <= 1));

    // Compute total number of PWM clock cycles required for the symbol duration
    const long int n_pwmclk_per_sym = std::round(F_PWM_CLK_INIT * tsym);

    // Counters for tracking transmitted PWM clocks and f0 occurrences
    long int n_pwmclk_transmitted = 0;
    long int n_f0_transmitted = 0;

    if (debug)
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
        while (accessBusAddress(DMA_BUS_BASE + 0x04 /* CurBlock */) == reinterpret_cast<long int>(instrs[bufPtr].b))
        {
            usleep(100);
        }
        reinterpret_cast<struct CB *>(instrs[bufPtr].v)->SOURCE_AD = reinterpret_cast<long int>(constPage.b) + f0_idx * 4;

        // Wait for f0 PWM clocks
        bufPtr++;
        while (accessBusAddress(DMA_BUS_BASE + 0x04 /* CurBlock */) == reinterpret_cast<long int>(instrs[bufPtr].b))
        {
            usleep(100);
        }
        reinterpret_cast<struct CB *>(instrs[bufPtr].v)->TXFR_LEN = n_f0;

        // Transmit frequency f1
        bufPtr++;
        while (accessBusAddress(DMA_BUS_BASE + 0x04 /* CurBlock */) == reinterpret_cast<long int>(instrs[bufPtr].b))
        {
            usleep(100);
        }
        reinterpret_cast<struct CB *>(instrs[bufPtr].v)->SOURCE_AD = reinterpret_cast<long int>(constPage.b) + f1_idx * 4;

        // Wait for f1 PWM clocks
        bufPtr = (bufPtr + 1) % 1024;
        while (accessBusAddress(DMA_BUS_BASE + 0x04 /* CurBlock */) == reinterpret_cast<long int>(instrs[bufPtr].b))
        {
            usleep(100);
        }
        reinterpret_cast<struct CB *>(instrs[bufPtr].v)->TXFR_LEN = n_f1;

        // Update transmission counters
        n_pwmclk_transmitted += n_pwmclk;
        n_f0_transmitted += n_f0;
    }

    if (debug)
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
void clear_dma_setup()
{
    // Turn off transmission.
    transmit_off();

    // Ensure memory-mapped peripherals are initialized before proceeding.
    if (dmaConfig.peripheral_base_virtual == nullptr)
    {
        return;
    }

    // Obtain a pointer to the DMA control registers.
    volatile DMAregs *DMA0 = reinterpret_cast<volatile DMAregs *>(&(accessBusAddress(DMA_BUS_BASE)));

    // Reset the DMA controller by setting the reset bit (bit 31) in the control/status register.
    DMA0->CS = 1 << 31;
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
        throw std::runtime_error("Error: Failed to open mailbox.");
    }
}

/**
 * @brief Safely removes a file if it exists.
 * @details Checks whether the specified file exists before attempting to remove it.
 *          If the file exists but removal fails, a warning is displayed.
 *
 * @param[in] filename Pointer to a null-terminated string containing the file path.
 */
void safe_remove()
{
    const char *filename = LOCAL_DEVICE_FILE_NAME;
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
 * @brief Clean up DMA and mailbox resources.
 *
 * @details Performs teardown in the following order:
 *   1. Prevent multiple invocations.
 *   2. Stop any ongoing DMA transfers and disable the PWM clock.
 *   3. Restore saved clock and PWM register values.
 *   4. Reset the DMA controller.
 *   5. Unmap the peripheral base address region.
 *   6. Deallocate mailbox memory pages.
 *   7. Close the mailbox handle.
 *   8. Remove the local device file.
 *   9. Reset all configuration data to defaults.
 *
 * @note This function is idempotent; subsequent calls are no‑ops.
 */
void dma_cleanup()
{
    // Guard against multiple calls
    static bool cleanup_done = false;
    if (cleanup_done)
    {
        return;
    }
    cleanup_done = true;

    // Stop DMA transfers and disable PWM clock
    transmit_off();

    // Restore original clock and PWM registers
    accessBusAddress(CM_GP0DIV_BUS) = dmaConfig.orig_gp0div;
    accessBusAddress(CM_GP0CTL_BUS) = dmaConfig.orig_gp0ctl;
    accessBusAddress(PWM_BUS_BASE + 0x00) = dmaConfig.orig_pwm_ctl;
    accessBusAddress(PWM_BUS_BASE + 0x04) = dmaConfig.orig_pwm_sta;
    accessBusAddress(PWM_BUS_BASE + 0x10) = dmaConfig.orig_pwm_rng1;
    accessBusAddress(PWM_BUS_BASE + 0x20) = dmaConfig.orig_pwm_rng2;
    accessBusAddress(PWM_BUS_BASE + 0x08) = dmaConfig.orig_pwm_fifocfg;

    // Reset DMA controller registers
    clear_dma_setup();

    // Unmap peripheral region if mapped
    if (dmaConfig.peripheral_base_virtual)
    {
        munmap(dmaConfig.peripheral_base_virtual, 0x01000000);
        dmaConfig.peripheral_base_virtual = nullptr;
    }

    // Deallocate mailbox-allocated memory pages
    deallocate_memory_pool();

    // Close mailbox handle if open
    if (mbox.handle >= 0)
    {
        mbox_close(mbox.handle);
        mbox.handle = -1;
    }

    // Remove the local device file if it exists
    safe_remove();

    // Reset global configuration structures to defaults
    dmaConfig = {};
    mbox = {};
}

/**
 * @brief Configures and initializes DMA for PWM signal generation.
 * @details Allocates memory pages, creates DMA control blocks, sets up a
 *          circular inked list of DMA instructions, and configures the
 *          PWM clock and registers.
 *
 * @param[out] constPage PageInfo structure for storing constant data.
 * @param[out] instrPage PageInfo structure for the initial DMA instruction
 *                       page.
 * @param[out] instrs Array of PageInfo structures for DMA instructions.
 */
void create_dma_pages(
    struct PageInfo &constPage,
    struct PageInfo &instrPage,
    struct PageInfo instrs[])
{
    // Allocate memory pool for DMA operation
    allocate_memory_pool(1025);

    // Allocate a memory page for storing constants
    get_real_mem_page_from_pool(&constPage.v, &constPage.b);

    // Initialize instruction counter
    int instrCnt = 0;

    // Allocate memory pages and create DMA instructions
    while (instrCnt < 1024)
    {
        // Allocate a memory page for instructions
        get_real_mem_page_from_pool(&instrPage.v, &instrPage.b);

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
    accessBusAddress(CLK_BUS_BASE + 40 * 4) = 0x5A000026; // Source = PLLD, disable
    usleep(1000);
    accessBusAddress(CLK_BUS_BASE + 41 * 4) = 0x5A002000; // Set PWM divider to 2 (250MHz)
    accessBusAddress(CLK_BUS_BASE + 40 * 4) = 0x5A000016; // Source = PLLD, enable
    usleep(1000);

    // Configure PWM registers
    accessBusAddress(PWM_BUS_BASE + 0x0) = 0; // Disable PWM
    usleep(1000);
    accessBusAddress(PWM_BUS_BASE + 0x4) = -1; // Clear status errors
    usleep(1000);
    accessBusAddress(PWM_BUS_BASE + 0x10) = 32; // Set default range
    accessBusAddress(PWM_BUS_BASE + 0x20) = 32;
    accessBusAddress(PWM_BUS_BASE + 0x0) = -1; // Enable FIFO mode, repeat, serializer, and channel
    usleep(1000);
    accessBusAddress(PWM_BUS_BASE + 0x8) = (1 << 31) | 0x0707; // Enable DMA

    // Obtain the base address as an integer pointer
    //
    // Compute the byte‐offset from the bus base (0x7E000000) to your desired
    // DMA register block (DMA_BUS_BASE).
    std::uintptr_t delta = DMA_BUS_BASE - 0x7E000000UL;
    //
    // Treat your void* as a char* so arithmetic is in bytes
    auto base = static_cast<char *>(dmaConfig.peripheral_base_virtual);
    //
    // Add the offset, then cast to your register pointer
    volatile int *dma_base = reinterpret_cast<volatile int *>(base + delta);

    // Cast to DMAregs pointer to activate DMA
    volatile struct DMAregs *DMA0 = reinterpret_cast<volatile struct DMAregs *>(dma_base);
    DMA0->CS = 1 << 31; // Reset DMA
    DMA0->CONBLK_AD = 0;
    DMA0->TI = 0;
    DMA0->CONBLK_AD = reinterpret_cast<unsigned long int>(instrPage.b);
    DMA0->CS = (1 << 0) | (255 << 16); // Enable DMA, priority level 255
}

/**
 * @brief Configure and initialize the DMA system for WSPR transmission.
 *
 * @details Performs the following steps in order:
 *   1. Retrieve and configure the PLLD clock frequency and DMA memory flag.
 *   2. Map the peripheral base address into user space.
 *   3. Save the original clock and PWM register values for later restoration.
 *   4. Open the Broadcom mailbox interface for DMA memory allocation.
 *   5. Allocate and set up DMA control blocks for constants and instruction pages.
 *
 * @throws std::runtime_error if the PLLD clock cannot be determined.
 * @throws std::runtime_error if peripheral memory mapping fails.
 * @throws std::runtime_error if mailbox opening fails.
 */
void setup_dma()
{
    // Retrieve PLLD frequency and DMA memory flag
    get_plld_and_memflag();

    // Map the peripheral base address into virtual memory
    setup_peripheral_base_virtual();

    // Save original register states for cleanup
    dmaConfig.orig_gp0ctl = accessBusAddress(CM_GP0CTL_BUS);
    dmaConfig.orig_gp0div = accessBusAddress(CM_GP0DIV_BUS);
    dmaConfig.orig_pwm_ctl = accessBusAddress(PWM_BUS_BASE + 0x00);
    dmaConfig.orig_pwm_sta = accessBusAddress(PWM_BUS_BASE + 0x04);
    dmaConfig.orig_pwm_rng1 = accessBusAddress(PWM_BUS_BASE + 0x10);
    dmaConfig.orig_pwm_rng2 = accessBusAddress(PWM_BUS_BASE + 0x20);
    dmaConfig.orig_pwm_fifocfg = accessBusAddress(PWM_BUS_BASE + 0x08);

    // Open the mailbox for DMA memory allocation
    open_mbox();

    // Allocate memory pages and build DMA control blocks
    create_dma_pages(constPage, instrPage, instrs);
}

/**
 * @brief Configures the DMA frequency table for signal generation.
 * @details Generates a tuning word table based on the desired center frequency
 *          and tone spacing, adjusting for hardware limitations if necessary.
 *
 * @param[in] center_freq_desired The desired center frequency in Hz.
 * @param[in] tone_spacing The spacing between frequency tones in Hz.
 * @param[in] plld_actual_freq The actual PLLD clock frequency in Hz.
 * @param[out] center_freq_actual The actual center frequency, which may be adjusted.
 * @param[in,out] constPage The PageInfo structure for storing tuning words.
 */
void setup_dma_freq_table(double &center_freq_actual)
{
    // Compute the divider values for the lowest and highest WSPR tones.
    double div_lo = bit_trunc(dmaConfig.plld_clock_frequency / (transParams.frequency - 1.5 * transParams.tone_spacing), -12) + std::pow(2.0, -12);
    double div_hi = bit_trunc(dmaConfig.plld_clock_frequency / (transParams.frequency + 1.5 * transParams.tone_spacing), -12);

    // If the integer portion of dividers differ, adjust the center frequency.
    if (std::floor(div_lo) != std::floor(div_hi))
    {
        center_freq_actual = dmaConfig.plld_clock_frequency / std::floor(div_lo) - 1.6 * transParams.tone_spacing;
        std::stringstream temp;
        temp << std::fixed << std::setprecision(6)
             << "Warning: center frequency has been changed to "
             << center_freq_actual / 1e6 << " MHz";
        std::cerr << temp.str() << " because of hardware limitations." << std::endl;
    }

    // Initialize tuning word table.
    double tone0_freq = center_freq_actual - 1.5 * transParams.tone_spacing;
    std::vector<long int> tuning_word(1024);

    // Generate tuning words for WSPR tones.
    for (int i = 0; i < 8; i++)
    {
        double tone_freq = tone0_freq + (i >> 1) * transParams.tone_spacing;
        double div = bit_trunc(dmaConfig.plld_clock_frequency / tone_freq, -12);

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
    for (int i = 0; i < 1024; i++)
    {
        transParams.dma_table_freq[i] = dmaConfig.plld_clock_frequency / (tuning_word[i] / std::pow(2.0, 12));

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
 * @brief Rebuild the DMA tuning‐word table with a fresh PPM correction.
 *
 * @param ppm_new The new parts‑per‑million offset (e.g. +11.135).
 * @throws std::runtime_error if peripherals aren’t mapped.
 */
void update_dma_for_ppm(double ppm_new)
{
    // Apply the PPM correction to your working PLLD clock.
    dmaConfig.plld_clock_frequency =
        dmaConfig.plld_nominal_freq * (1.0 - ppm_new / 1e6);

    // Recompute the DMA frequency table in place.
    // Pass in your current center frequency so it can adjust if needed.
    double center_actual = transParams.frequency;
    setup_dma_freq_table(center_actual);
    transParams.frequency = center_actual;
}

/**
 * @brief Configure and start a WSPR transmission.
 *
 * @details Performs the following sequence:
 *   1. Set the desired RF frequency and power level.
 *   2. Populate WSPR symbol data if transmitting a message.
 *   3. Determine WSPR mode (2‑tone or 15‑tone) and symbol timing.
 *   4. Optionally apply a random frequency offset to spread spectral load.
 *   5. Initialize DMA and mailbox resources.
 *   6. Apply the specified PPM calibration to the PLLD clock.
 *   7. Rebuild the DMA frequency table with the new PPM‑corrected clock.
 *   8. Update the actual center frequency after any hardware adjustments.
 *
 * @param[in] frequency    Target RF frequency in Hz.
 * @param[in] power        Transmit power index (0‑n).
 * @param[in] ppm          Parts‑per‑million correction to apply (e.g. +11.135).
 * @param[in] callsign     Optional callsign for WSPR message.
 * @param[in] grid_square  Optional Maidenhead grid locator.
 * @param[in] power_dbm    dBm value for WSPR message (ignored if tone).
 * @param[in] use_offset   True to apply a small random offset within band.
 *
 * @throws std::runtime_error if DMA setup or mailbox operations fail.
 */
void setup_transmission(
    double frequency,
    int power,
    double ppm,
    const std::string &callsign,
    const std::string &grid_square,
    int power_dbm,
    bool use_offset)
{
    // Set RF frequency and power parameters
    transParams.frequency = frequency;
    transParams.power = power;

    // Default to tone‑only; load WSPR message if provided
    transParams.is_tone = true;
    if (!callsign.empty() && !grid_square.empty() && power_dbm != 0)
    {
        transParams.is_tone = false;
        WsprMessage msg(callsign, grid_square, power_dbm);
        std::copy_n(msg.symbols, msg.size, transParams.symbols.begin());
    }

    // Choose WSPR mode and symbol timing
    int offset_freq = 0;
    if (!transParams.is_tone &&
        ((frequency > 137600 && frequency < 137625) ||
         (frequency > 475800 && frequency < 475825) ||
         (frequency > 1838200 && frequency < 1838225)))
    {
        // WSPR‑15 mode
        transParams.wspr_mode = WsprMode::WSPR15;
        transParams.symtime = 8.0 * WSPR_SYMTIME;
        if (use_offset)
            offset_freq = WSPR15_RAND_OFFSET;
    }
    else
    {
        // WSPR‑2 mode
        transParams.wspr_mode = WsprMode::WSPR2;
        transParams.symtime = WSPR_SYMTIME;
        if (use_offset)
            offset_freq = WSPR_RAND_OFFSET;
    }
    transParams.tone_spacing = 1.0 / transParams.symtime;

    // Apply random offset if requested
    if (use_offset)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(-1.0, 1.0);
        transParams.frequency += dis(gen) * offset_freq;
    }

    // Initialize DMA, mapping, and control blocks
    setup_dma();

    // Apply PPM correction to the PLLD clock
    // (use a stored nominal base frequency for repeatable resets)
    dmaConfig.plld_clock_frequency =
        dmaConfig.plld_nominal_freq * (1 - ppm / 1e6);

    // Build the DMA frequency lookup table with new clock
    double center_actual = transParams.frequency;
    setup_dma_freq_table(center_actual);

    // Update actual frequency after any hardware adjustments
    transParams.frequency = center_actual;

    // Optional debug output
    if (debug)
    {
        std::cout << std::setprecision(30)
                  << "DEBUG: dma_table_freq[0] = " << transParams.dma_table_freq[0] << std::endl
                  << "DEBUG: dma_table_freq[1] = " << transParams.dma_table_freq[1] << std::endl
                  << "DEBUG: dma_table_freq[2] = " << transParams.dma_table_freq[2] << std::endl
                  << "DEBUG: dma_table_freq[3] = " << transParams.dma_table_freq[3] << std::endl;
        // Debug output for transmission
        transParams.print();
    }
}

/**
 * @brief Perform DMA-driven RF transmission.
 *
 * @details
 *   1. Records the start time as a reference for symbol scheduling.
 *   2. Enables the PWM clock and DMA engine for transmission.
 *   3. If in tone mode, continuously transmits a fixed‑frequency test tone.
 *   4. Otherwise, transmits each WSPR symbol in sequence, using gettimeofday()
 *      and `timeval_subtract()` to schedule precise symbol timing.
 *   5. Disables transmission when complete.
 *
 * @note In tone mode (`transParams.is_tone == true`), this function only
 *       returns via SIGINT.
 */
void transmit()
{
    // Record reference time for scheduling
    struct timeval tv_begin{};
    gettimeofday(&tv_begin, nullptr);

    // Initialize DMA buffer index
    int bufPtr = 0;

    // Enable PWM clock and DMA transmission
    transmit_on();

    if (transParams.is_tone)
    {
        // Continuous tone loop (exit on SIGINT)
        while (true)
        {
            transmit_symbol(
                0,     // symbol number
                60.0,  // duration in seconds
                bufPtr // DMA buffer index
            );
        }
    }
    else
    {
        // Transmit each symbol in the WSPR message
        struct timeval sym_start{};
        struct timeval diff{};
        const int symbol_count = static_cast<int>(transParams.symbols.size());

        for (int i = 0; i < symbol_count; ++i)
        {
            // Compute elapsed time since tv_begin
            gettimeofday(&sym_start, nullptr);
            symbol_timeval_subtract(&diff, &sym_start, &tv_begin);
            double elapsed = diff.tv_sec + diff.tv_usec / 1e6;
            double sched_end = (i + 1) * transParams.symtime;
            double time_symbol = sched_end - elapsed;
            time_symbol = std::clamp(time_symbol, 0.2, 2.0 * transParams.symtime);

            // Transmit the current symbol
            int symbol = static_cast<int>(transParams.symbols[i]);
            transmit_symbol(
                symbol,      // symbol index
                time_symbol, // scheduled duration
                bufPtr       // DMA buffer index
            );
        }
    }

    // Disable PWM clock and stop transmission
    transmit_off();
}
