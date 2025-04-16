#include "wspr_transmit.hpp"

// Project Headers
#include "config_handler.hpp"
#include "utils.hpp"
#include "wspr_transmit.hpp"

// Submodules
#include "wspr_constants.hpp"
#include "wspr_structs.hpp"
#include "wspr_message.hpp"

// C++ Standard Library Headers
#include <array>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <termios.h>
#include <vector>

// C Standard Library Headers
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

// POSIX & System-Specific Headers
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timex.h>

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
    // Calculate the offset into the peripheral space.
    std::uintptr_t offset = bus_addr - 0x7e000000UL;
    // Add the offset to the base virtual address, then reinterpret the resulting address.
    return *reinterpret_cast<volatile int *>(
        reinterpret_cast<std::uintptr_t>(dmaConfig.peri_base_virt) + offset);
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
 * @brief Initializes DMA configuration for PLLD clock settings.
 *
 * Determines the Raspberry Pi hardware revision by reading from
 * `/proc/cpuinfo`, extracts the processor ID, and sets the appropriate
 * DMA memory flag and PLLD clock frequency in `dmaConfig`.
 *
 * This function configures:
 * - `dmaConfig.mem_flag` based on the board's memory allocation strategy
 * - `dmaConfig.plld_clock_frequency` based on the processor's clock
 *
 * Supported processors:
 * - BCM2835 (Raspberry Pi 1)
 * - BCM2836 / BCM2837 (Raspberry Pi 2 / 3)
 * - BCM2711 (Raspberry Pi 4)
 *
 * @note The board revision is cached after first read to avoid repeated
 *       filesystem access.
 *
 * @throws Terminates the program with `std::exit(EXIT_FAILURE)` if the
 *         processor ID is unrecognized.
 *
 * @warning If the PLLD frequency cannot be determined or is invalid, a
 *          default of 500 MHz is used as a fallback.
 */
void get_plld_and_memflag()
{
    // Cache the revision to avoid reading the file multiple times
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
            cached_revision = 0; // Default if reading/parsing failed
        }
    }

    const unsigned rev = *cached_revision;

    // Determine processor ID
    const int processor_id = (rev & 0x800000) ? ((rev & 0xf000) >> 12) : BCM_HOST_PROCESSOR_BCM2835;

    // Configure PLLD and mem_flag based on processor ID
    switch (processor_id)
    {
    case BCM_HOST_PROCESSOR_BCM2835: // Raspberry Pi 1
        dmaConfig.mem_flag = 0x0c;
        dmaConfig.plld_clock_frequency = 500000000.0 * (1 - 2.500e-6); // 2.5 PPM correction
        break;
    case BCM_HOST_PROCESSOR_BCM2836: // Raspberry Pi 2
    case BCM_HOST_PROCESSOR_BCM2837: // Raspberry Pi 3
        dmaConfig.mem_flag = 0x04;
        dmaConfig.plld_clock_frequency = 500000000.0;
        break;
    case BCM_HOST_PROCESSOR_BCM2711: // Raspberry Pi 4
        dmaConfig.mem_flag = 0x04;
        dmaConfig.plld_clock_frequency = 750000000.0;
        break;
    default:
        throw std::runtime_error("Error: Unknown chipset (" + std::to_string(processor_id) + ").");
    }

    if (dmaConfig.plld_clock_frequency <= 0)
    {
        std::cerr << "Error: Invalid PLL clock frequency. Using default 500 MHz." << std::endl;
        dmaConfig.plld_clock_frequency = 500000000.0;
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
 * @param[out] dmaConfig.peri_base_virt Reference to a pointer that will
 *             be set to the mapped virtual memory address.
 *
 * @throws Terminates the program if the peripheral base cannot be determined,
 *         `/dev/mem` cannot be opened, or `mmap` fails.
 */
void setup_peri_base_virt()
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

        // Big-endian to host-endian conversion
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

    dmaConfig.peri_base_virt = static_cast<unsigned *>(mmap(
        nullptr,
        0x01000000,             // 16MB memory region
        PROT_READ | PROT_WRITE, // Allow read and write
        MAP_SHARED,
        mem_fd,
        peripheral_base // Physical address to map
        ));

    if (reinterpret_cast<long int>(dmaConfig.peri_base_virt) == -1)
    {
        throw std::runtime_error("Error: peri_base_virt mmap failed.");
    }

    close(mem_fd);
}

/**
 * @brief Allocates a memory pool for DMA usage.
 * @details Uses the mailbox interface to allocate a contiguous block of memory.
 *          The memory is locked down, mapped to virtual user space, and its
 *          bus address is stored for further use.
 *
 * @param numpages Number of memory pages to allocate.
 */
void allocate_memory_pool(unsigned numpages) // TODO:  Why 1025 here?
{
    // Allocate a contiguous block of memory using the mailbox interface.
    mbox.mem_ref = mem_alloc(mbox.handle, PAGE_SIZE * numpages, BLOCK_SIZE, dmaConfig.mem_flag);

    // Lock the allocated memory block and retrieve its bus address.
    mbox.bus_addr = mem_lock(mbox.handle, mbox.mem_ref);

    // Convert the bus address to a physical address and map it to user space.
    mbox.virt_addr = static_cast<unsigned char *>(mapmem(busToPhys(mbox.bus_addr), PAGE_SIZE * numpages));

    // Store the total number of pages allocated in the pool (constant for its lifetime).
    mbox.pool_size = numpages;

    // Initialize the count of used pages in the pool.
    mbox.pool_cnt = 0;

    if (debug)
    {
        // Debug print: Displays memory allocation details in hexadecimal format.
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
    if (dmaConfig.peri_base_virt == nullptr)
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
    accessBusAddress(PADS_GPIO_0_27_BUS) = 0x5a000018 + config.power_level;

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
 * @param[in] center_freq The center frequency in Hz.
 * @param[in] tone_spacing The frequency spacing between tones in Hz.
 * @param[in] tsym The duration (seconds) for which the symbol is transmitted.
 * @param[in] f_pwm_clk The frequency of the PWM clock in Hz.
 * @param[in,out] instrs The DMA instruction page array.
 * @param[in] constPage The memory page containing constant data.
 * @param[in,out] bufPtr The buffer pointer index for DMA instruction handling.
 */
void transmit_symbol(
    const int &sym_num,
    const double &center_freq,
    const double &tone_spacing,
    const double &tsym,
    const double &f_pwm_clk,
    struct PageInfo instrs[],
    struct PageInfo &constPage,
    int &bufPtr)
{
    // Determine indices for DMA frequency table
    const int f0_idx = sym_num * 2;
    const int f1_idx = f0_idx + 1;

    // Retrieve frequency values from the DMA table
    const double f0_freq = transParams.dma_table_freq[f0_idx];
    const double f1_freq = transParams.dma_table_freq[f1_idx];

    // Calculate the expected tone frequency
    const double tone_freq = center_freq - 1.5 * tone_spacing + sym_num * tone_spacing;

    // Ensure the tone frequency is within bounds
    assert((tone_freq >= f0_freq) && (tone_freq <= f1_freq));

    // Compute frequency ratio for symbol transmission
    const double f0_ratio = 1.0 - (tone_freq - f0_freq) / (f1_freq - f0_freq);

    if (debug)
        std::cout << "DEBUG: f0_ratio = " << f0_ratio << std::endl;

    // Ensure f0_ratio is between 0.0 and 1.0
    assert((f0_ratio >= 0) && (f0_ratio <= 1));

    // Compute total number of PWM clock cycles required for the symbol duration
    const long int n_pwmclk_per_sym = std::round(f_pwm_clk * tsym);

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
    if (dmaConfig.peri_base_virt == nullptr)
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
    if (cleanup_done)
        return; // Prevent duplicate cleanup
    cleanup_done = true;

    // Disable the clock to prevent unintended operation
    disable_clock();

    // Reset and clean up DMA-related settings
    clear_dma_setup();

    // Deallocate memory pool to free allocated resources
    deallocate_memory_pool();

    // Remove the local device file
    safe_remove(LOCAL_DEVICE_FILE_NAME);
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

/**
 * @brief Configures and initializes the DMA system for transmission.
 * @details Retrieves PLLD frequency, calibrates timing, sets scheduling priority,
 *          initializes random number generator, validates the frequency setting,
 *          and sets up DMA with peripherals.
 */
void setup_dma()
{
    // Get fresh PPM
    config.ppm = get_ppm_from_chronyc();

    // Retrieve PLLD frequency
    get_plld_and_memflag();

    // Initialize peripheral memory mapping
    setup_peri_base_virt();

    // Open mailbox for communication
    open_mbox();

    // Configure DMA for transmission
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
void setup_dma_freq_table(
    const double &center_freq_desired,
    const double &tone_spacing,
    const double &plld_actual_freq,
    double &center_freq_actual,
    struct PageInfo &constPage)
{
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
    for (int i = 0; i < 1024; i++)
    {
        transParams.dma_table_freq[i] = plld_actual_freq / (tuning_word[i] / std::pow(2.0, 12));

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
 * @brief Continuously transmits a test tone.
 * @details Generates a single test tone using the DMA setup, continuously transmitting it
 *          until the user exits with `CTRL-C`. The tone frequency is dynamically adjusted
 *          based on PPM calibration.
 */
void transmit_tone(double frequency)
{
    // Set operating frequency
    transParams.frequency = frequency;

    // Define WSPR symbol time and tone spacing
    transParams.symtime = WSPR_SYMTIME;
    transParams.tone_spacing = 1.0 / transParams.symtime;

    setup_dma();

    // Get adjustments based on PPM
    config.ppm = get_ppm_from_chronyc();
    // Compute actual PLL-adjusted frequency
    double adjusted_plld_freq = dmaConfig.plld_clock_frequency * (1 - config.ppm / 1e6);

    // Hold returned actual frequency
    double center_freq_actual = transParams.frequency;

    // Setup the DMA frequency table
    setup_dma_freq_table(
        transParams.frequency + 1.5 * transParams.tone_spacing,
        transParams.tone_spacing,
        adjusted_plld_freq,
        center_freq_actual,
        constPage);
    transParams.frequency = center_freq_actual;

    if (debug)
    {
        // Debug output for transmission
        std::cout << std::setprecision(30)
                  << "DEBUG: dma_table_freq[0] = " << transParams.dma_table_freq[0] << std::endl
                  << "DEBUG: dma_table_freq[1] = " << transParams.dma_table_freq[1] << std::endl
                  << "DEBUG: dma_table_freq[2] = " << transParams.dma_table_freq[2] << std::endl
                  << "DEBUG: dma_table_freq[3] = " << transParams.dma_table_freq[3] << std::endl;
        transParams.print();
    }

    std::cout << "Setup complete, transmitting." << std::endl;
    std::cout << "Press CTRL-C to end." << std::endl;

    // Time structs for computing transmission window
    struct timeval tvBegin, tvEnd, tvDiff;
    gettimeofday(&tvBegin, NULL);
    std::cout << "TX started at: " << timeval_print(&tvBegin) << std::endl;

    // DMA buffer pointer
    int bufPtr = 0;

    // Enable transmission mode
    transmit_on();

    // Continuous transmission loop
    while (true)
    {
        // Transmit the test tone symbol
        transmit_symbol(
            0,                        // The symbol number to transmit.
            transParams.frequency,    // The center frequency in Hz.
            transParams.tone_spacing, // The frequency spacing between tones in Hz.
            60,                       // The duration (seconds) for which the symbol is transmitted.
            F_PWM_CLK_INIT,           // The frequency of the PWM clock in Hz.
            instrs,                   // The DMA instruction page array.
            constPage,                // The memory page containing constant data.
            bufPtr                    // The buffer pointer index for DMA instruction handling.
        );
    }

    // Disable transmission mode
    transmit_off();

    // Print transmission timestamp and duration
    gettimeofday(&tvEnd, nullptr);
    timeval_subtract(&tvDiff, &tvEnd, &tvBegin);
    std::cout << "TX ended at: " << timeval_print(&tvEnd)
              << " (" << tvDiff.tv_sec << "." << std::setfill('0') << std::setw(3)
              << (tvDiff.tv_usec + 500) / 1000 << " s)" << std::endl;
}

/**
 * @brief Transmits WSPR (Weak Signal Propagation Reporter) messages.
 * @details Continuously loops through available bands, waiting for WSPR transmission
 *          windows, generating WSPR symbols, and transmitting them.
 *          The function dynamically adjusts transmission frequency based on
 *          PPM calibration and ensures accurate symbol timing.
 */
void transmit_wspr(std::string callsign, std::string grid_square, int power_dbm, double frequency, bool use_offset)
{
    // Set operating frequency
    transParams.frequency = frequency;

    // Create a WSPR message instance
    WsprMessage wMessage(callsign, grid_square, power_dbm);
    std::copy_n(wMessage.symbols, wMessage.size, transParams.symbols.begin());

    // Define WSPR symbol time and tone spacing per WSPR mode (2 vs 15)
    int offset_freq = 0;
    if (
        (transParams.frequency > 137600 && transParams.frequency < 137625) ||
        (transParams.frequency > 475800 && transParams.frequency < 475825) ||
        (transParams.frequency > 1838200 && transParams.frequency < 1838225))
    {
        // For WSPR15
        transParams.wspr_mode = WsprMode::WSPR15;
        transParams.symtime = 8.0 * WSPR_SYMTIME;
        if (use_offset)
        {
            offset_freq = WSPR15_RAND_OFFSET;
        }
    }
    else
    {
        // For WSPR2
        transParams.wspr_mode = WsprMode::WSPR2;
        transParams.symtime = WSPR_SYMTIME;
        if (use_offset)
        {
            offset_freq = WSPR_RAND_OFFSET;
        }
    }
    transParams.tone_spacing = 1.0 / transParams.symtime;

    // Apply random offset if enabled
    if (use_offset)
    {
        std::random_device rd;
        std::mt19937 gen(rd()); // Seed with a fixed value for repeatability
        std::uniform_real_distribution<> dis(-1.0, 1.0);
        transParams.frequency += dis(gen) * offset_freq;
    }

    setup_dma();

    // Get adjustments based on PPM
    config.ppm = get_ppm_from_chronyc();
    // Compute actual PLL-adjusted frequency
    double adjusted_plld_freq = dmaConfig.plld_clock_frequency * (1 - config.ppm / 1e6);

    // Hold returned actual frequency
    double center_freq_actual = transParams.frequency;

    setup_dma_freq_table(
        transParams.frequency,
        transParams.tone_spacing,
        adjusted_plld_freq,
        center_freq_actual,
        constPage);
    transParams.frequency = center_freq_actual;

    if (debug)
    {
        // Debug output for transmission
        std::cout << std::setprecision(30)
                  << "DEBUG: dma_table_freq[0] = " << transParams.dma_table_freq[0] << std::endl
                  << "DEBUG: dma_table_freq[1] = " << transParams.dma_table_freq[1] << std::endl
                  << "DEBUG: dma_table_freq[2] = " << transParams.dma_table_freq[2] << std::endl
                  << "DEBUG: dma_table_freq[3] = " << transParams.dma_table_freq[3] << std::endl;
        transParams.print();
    }

    std::cout << "Ready to transmit (setup complete)." << std::endl;
    std::cout << "Waiting for next transmission window." << std::endl;
    std::cout << "Press <spacebar> to start immediately." << std::endl;
    waitForOneSecondPastEvenMinute();

    // Time structs for computing transmission window
    struct timeval tvBegin, tvEnd, tvDiff;
    gettimeofday(&tvBegin, NULL);
    std::cout << "TX started at: " << timeval_print(&tvBegin) << std::endl;

    // Structures for symbol timing
    struct timeval sym_start, diff;

    // DMA buffer pointer
    int bufPtr = 0;

    // Enable transmission
    transmit_on();

    // Transmit each symbol in the WSPR message
    for (int i = 0; i < static_cast<int>(transParams.symbols.size()); i++)
    {
        // Symbol timing
        gettimeofday(&sym_start, NULL);
        timeval_subtract(&diff, &sym_start, &tvBegin);
        double elapsed = diff.tv_sec + diff.tv_usec / 1e6;
        double sched_end = (i + 1) * transParams.symtime;
        double time_symbol = sched_end - elapsed;
        time_symbol = std::clamp(time_symbol, 0.2, 2 * transParams.symtime);
        int symbol = static_cast<int>(transParams.symbols[i]);

        transmit_symbol(
            symbol,                   // The symbol number to transmit.
            transParams.frequency,    // The center frequency in Hz.
            transParams.tone_spacing, // The frequency spacing between tones in Hz.
            time_symbol,              // The duration (seconds) for which the symbol is transmitted.
            F_PWM_CLK_INIT,           // The frequency of the PWM clock in Hz.
            instrs,                   // The DMA instruction page array.
            constPage,                // The memory page containing constant data.
            bufPtr                    // The buffer pointer index for DMA instruction handling.
        );
    }

    // Disable transmission
    transmit_off();

    // Print transmission timestamp and duration
    gettimeofday(&tvEnd, nullptr);
    timeval_subtract(&tvDiff, &tvEnd, &tvBegin);
    std::cout << "TX ended at: " << timeval_print(&tvEnd)
              << " (" << tvDiff.tv_sec << "." << std::setfill('0') << std::setw(3)
              << (tvDiff.tv_usec + 500) / 1000 << " s)" << std::endl;
}
