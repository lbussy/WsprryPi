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
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-32d490b [refactoring].
*/

#include "version.hpp"
#include "singleton.hpp"
#include "monitorfile.hpp"
#include "config.hpp"
#include "lcblog.hpp"
#include "utils.hpp"
#include "wspr_message.hpp"

#include "main.hpp"

// #define WSPR_DEBUG

// TCP port to bind to check for Singleton
#define SINGLETON_PORT 1234

// Logging library
LCBLog llog;

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

#define PAGE_SIZE (4 * 1024)
#define BLOCK_SIZE (4 * 1024)

#define PWM_CLOCKS_PER_ITER_NOMINAL 1000

// Given an address in the bus address space of the peripherals, this
// macro calculates the appropriate virtual address to use to access
// the requested bus address space. It does this by first subtracting
// 0x7e000000 from the supplied bus address to calculate the offset into
// the peripheral address space. Then, this offset is added to peri_base_virt
// Which is the base address of the peripherals, in virtual address space.
#define ACCESS_BUS_ADDR(buss_addr) *(volatile int *)((long int)peri_base_virt + (buss_addr)-0x7e000000)
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

// Used for GPIO DIO Access:
// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio + ((g) / 10)) &= ~(7 << (((g) % 10) * 3))
#define OUT_GPIO(g) *(gpio + ((g) / 10)) |= (1 << (((g) % 10) * 3))
#define SET_GPIO_ALT(g, a) *(gpio + (((g) / 10))) |= (((a) <= 3 ? (a) + 4 : (a) == 4 ? 3 : 2) << (((g) % 10) * 3))
#define GPIO_SET *(gpio + 7)    // sets bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio + 10) // clears bits which are 1 ignores bits which are 0
#define GET_GPIO(g) (*(gpio + 13) & (1 << g)) // 0 if LOW, (1<<g) if HIGH
#define GPIO_PULL *(gpio + 37) // Pull up/pull down
#define GPIO_PULLCLK0 *(gpio + 38) // Pull up/pull down clock

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

// Class to monitor for file changes
MonitorFile iniMonitor;

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
    std::string inifile;
    bool xmit_enabled = false;
    bool repeat = false;
    std::string callsign;
    std::string grid_square;
    std::string tx_power;
    std::string frequency_string;
    std::vector<double> center_freq_set;
    double ppm = 0;
    bool self_cal = true;
    bool random_offset = false;
    double test_tone = NAN;
    bool no_delay = false;
    mode_type mode = WSPR;
    int terminate = -1;
    bool use_led = false;
    bool daemon_mode = false;
    int power_level = 7;
    // PLLD clock frequency.
    double f_plld_clk;
    // MEM_FLAG_L1_NONALLOCATING?
    int mem_flag;
} config;

// GPIO/DIO Control:

void setupGPIO(int pin = 0)
{
    if (pin == 0) return;
    // Set up gpio pointer for direct register access
    int mem_fd;
    // Set up a memory regions to access GPIO
    unsigned gpio_base = gpioBase() + 0x200000;

    if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
    {
        llog.logE("Fail: Unable to open /dev/mem (running as root?)");
        exit(-1);
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
        exit(-1);
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
    if (pin == 0) return;
    GPIO_SET = 1 << pin;
}

void pinLow(int pin = 0)
{
    if (pin == 0) return;
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

    switch (ver())
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
        fprintf(stderr, "Error: Unknown chipset (%d).", ver());
        exit(-1);
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
        llog.logE("Error: unable to allocated more pages.");
        exit(-1);
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

void txon(bool led = false, int power_level = 7)
{
    // Turn on TX
    if (led) pinHigh(LED_PIN);
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
    switch (power_level)  {
    case 0:
        ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 0;  //2mA -3.4dBm
        break;
    case 1:
        ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 1;  //4mA +2.1dBm
        break;
    case 2:
        ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 2;  //6mA +4.9dBm
        break;
    case 3:
        ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 3;  //8mA +6.6dBm(default)
        break;
    case 4:
        ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 4;  //10mA +8.2dBm
        break;
    case 5:
        ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 5;  //12mA +9.2dBm
        break;
    case 6:
        ACCESS_BUS_ADDR(PADS_GPIO_0_27_BUS) = 0x5a000018 + 6;  //14mA +10.0dBm
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

void txoff(bool led = false)
{
    // Turn transmitter on
    // struct GPCTL setupword = {6/*SRC*/, 0, 0, 0, 0, 1,0x5a};
    // ACCESS_BUS_ADDR(CM_GP0CTL_BUS) = *((int*)&setupword);
    if (led) pinLow(LED_PIN);
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
    // cout << "Exiting." << std::endl;
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
        temp << std::setprecision(6) << std::fixed << "Warning: center frequency has been changed to " << center_freq_actual / 1e6 << " MHz";
        llog.logE(temp.str(), " because of hardware limitations.");
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

void print_usage()
{
    llog.logS("Usage:");
    llog.logS("  wspr [options] callsign gridsquare tx_pwr_dBm f1 <f2> <f3> ...");
    llog.logS("    OR");
    llog.logS("  wspr [options] --test-tone f");
    llog.logS("");
    llog.logS("Options:");
    llog.logS("  -h --help");
    llog.logS("    Print out this help screen.");
    llog.logS("  -v --version");
    llog.logS("    Show the Wsprry Pi version.");
    llog.logS("  -p --ppm ppm");
    llog.logS("    Known PPM correction to 19.2MHz RPi nominal crystal frequency.");
    llog.logS("  -s --self-calibration");
    llog.logS("    Check NTP before every transmission to obtain the PPM error of the");
    llog.logS("    crystal (default setting.)");
    llog.logS("  -f --free-running");
    llog.logS("    Do not use NTP to correct the frequency error of RPi crystal.");
    llog.logS("  -r --repeat");
    llog.logS("    Repeatedly and in order, transmit on all the specified command line");
    llog.logS("    freqs.");
    llog.logS("  -x --terminate <n>");
    llog.logS("    Terminate after completing <n> transmissions.");
    llog.logS("  -o --offset");
    llog.logS("    Add a random frequency offset to each transmission:");
    llog.logS("      +/- ", WSPR_RAND_OFFSET, "Hz for WSPR");
    llog.logS("      +/- ", WSPR15_RAND_OFFSET, "Hz for WSPR-15");
    llog.logS("  -t --test-tone freq");
    llog.logS("    Output a test tone at the specified frequency. Only used for");
    llog.logS("    debugging and verifying calibration.");
    llog.logS("  -l --led");
    llog.logS("    Use LED as a transmit indicator (TAPR board).");
    llog.logS("  -n --no-delay;");
    llog.logS("    Transmit immediately without waiting for a WSPR TX window. Used for");
    llog.logS("    testing only.");
    llog.logS("  -i --ini-file");
    llog.logS("    Load parameters from an ini file. Supply path and file name.");
    llog.logS("  -D --daemon-mode");
    llog.logS("    Run with terse messaging.");
    llog.logS("  -d --power_level");
    llog.logS("    Set actual TX power, 0-7.");
    llog.logS("");
    llog.logS("Frequencies can be specified either as an absolute TX carrier frequency,");
    llog.logS("or using one of the following bands:");
    llog.logS("");
    llog.logS("  LF, LF-15, MF, MF-15, 160m, 160m-15, 80m, 60m, 40m, 30m, 20m,");
    llog.logS("  17m, 15m, 12m, 10m, 6m, 4m, and 2m");
    llog.logS("");
    llog.logS("If you specify a band, the transmission will happen in the middle of the");
    llog.logS("WSPR region of the selected band.");
    llog.logS("");
    llog.logS("The \"-15\" suffix indicates the WSPR-15 region of the band.");
    llog.logS("");
    llog.logS("You may create transmission gaps by specifying a TX frequency of 0.");
    llog.logS("");
}

bool getINIValues(bool reload = false)
{
    WSPRConfig iniConfig;
    if (iniConfig.initialize(config.inifile))
    {
        config.xmit_enabled = iniConfig.getTransmit();
        config.callsign = iniConfig.getCallsign();
        config.grid_square = iniConfig.getGridsquare();
        config.tx_power = iniConfig.getTxpower();
        config.frequency_string.clear(); // Ensure previous data is cleared
        config.frequency_string = iniConfig.getFrequency(); // Assign the new value
        config.ppm = iniConfig.getPpm();
        config.self_cal = iniConfig.getSelfcal();
        config.random_offset = iniConfig.getOffset();
        config.use_led = iniConfig.getUseLED();
        config.power_level = iniConfig.getPowerLevel();

        if (! config.daemon_mode )
            llog.logS("\n============================================");
        llog.logS("Config ", ((reload) ? "re" : ""), "loaded from: ", config.inifile);
        if (! config.daemon_mode )
            llog.logS("============================================");
        llog.logS("Transmit Enabled:\t\t", ((config.xmit_enabled) ? "true" : "false"));
        llog.logS("Call Sign:\t\t\t", config.callsign);
        llog.logS("Grid Square:\t\t\t", config.grid_square);
        llog.logS("Transmit Power:\t\t\t", config.tx_power);
        llog.logS("Frequencies:\t\t\t", config.frequency_string);
        llog.logS("PPM Offset:\t\t\t", config.ppm);
        llog.logS("Do not use NTP sync:\t\t", ((!config.self_cal) ? "true" : "false"));
        llog.logS("Check NTP Each Run (default):\t", ((config.self_cal) ? "true" : "false"));
        llog.logS("Use Frequency Randomization:\t", ((config.random_offset) ? "true" : "false"));
        llog.logS("Power Level:\t\t\t", config.power_level);
        llog.logS("Use LED:\t\t\t", ((config.use_led) ? "true" : "false"));
        if (! config.daemon_mode )
            llog.logS("============================================\n");
        return true;
    }
    else
    {
        return false;
    }

}

void convertToFreq(const char* &option, double &parsed_freq)
{
    if (!strcasecmp(option, "LF"))
    {
        parsed_freq = 137500.0;
    }
    else if (!strcasecmp(option, "LF-15"))
    {
        parsed_freq = 137612.5;
    }
    else if (!strcasecmp(option, "MF"))
    {
        parsed_freq = 475700.0;
    }
    else if (!strcasecmp(option, "MF-15"))
    {
        parsed_freq = 475812.5;
    }
    else if (!strcasecmp(option, "160m"))
    {
        parsed_freq = 1838100.0;
    }
    else if (!strcasecmp(option, "160m-15"))
    {
        parsed_freq = 1838212.5;
    }
    else if (!strcasecmp(option, "80m"))
    {
        parsed_freq = 3570100.0;
    }
    else if (!strcasecmp(option, "60m"))
    {
        parsed_freq = 5288700.0;
    }
    else if (!strcasecmp(option, "40m"))
    {
        parsed_freq = 7040100.0;
    }
    else if (!strcasecmp(option, "30m"))
    {
        parsed_freq = 10140200.0;
    }
    else if (!strcasecmp(option, "20m"))
    {
        parsed_freq = 14097100.0;
    }
    else if (!strcasecmp(option, "17m"))
    {
        parsed_freq = 18106100.0;
    }
    else if (!strcasecmp(option, "15m"))
    {
        parsed_freq = 21096100.0;
    }
    else if (!strcasecmp(option, "12m"))
    {
        parsed_freq = 24926100.0;
    }
    else if (!strcasecmp(option, "10m"))
    {
        parsed_freq = 28126100.0;
    }
    else if (!strcasecmp(option, "6m"))
    {
        parsed_freq = 50294500.0;
    }
    else if (!strcasecmp(option, "4m"))
    {
        parsed_freq = 70092500.0;
    }
    else if (!strcasecmp(option, "2m"))
    {
        parsed_freq = 144490500.0;
    }
    else
    {
        // Not a string. See if it can be parsed as a double.
        char *endp;
        parsed_freq = strtod(option, &endp);
        if ((optarg == endp) || (*endp != '\0'))
        {
            llog.logE("Error: Could not parse transmit frequency: ", option);
            exit(-1);
        }
    }
}

bool parse_commandline(const int &argc, char *const argv[])
{
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {"ppm", required_argument, 0, 'p'},
        {"self-calibration", no_argument, 0, 's'},
        {"free-running", no_argument, 0, 'f'},
        {"repeat", no_argument, 0, 'r'},
        {"terminate", required_argument, 0, 'x'},
        {"offset", no_argument, 0, 'o'},
        {"test-tone", required_argument, 0, 't'},
        {"no-delay", no_argument, 0, 'n'},
        {"led", no_argument, 0, 'l'},
        {"ini-file", required_argument, 0, 'i'},
        {"daemon-mode", no_argument, 0, 'D'},
        {"power_level", required_argument, 0, 'd'},
        {0, 0, 0, 0}};

    while (true)
    {
        /* getopt_long stores the option index here. */
        int option_index = 0;
        int c = getopt_long(argc, argv, "?vhp:sfrxd:ot:nli:D",
                            long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
            char *endp;
        case 0:
            // Code should only get here if a long option was given a non-null
            // flag value.
            llog.logE("Check code.");
            return false;
            break;
        case 'h':
        case '?':
            // Help
            print_usage();
            return false;
            break;
        case 'v':
            // Version
            llog.logS("Wsprry Pi (wspr) version ", exeversion(), " (", branch(), ").");
            return false;
            break;
        case 'p':
            // Set PPM to X
            config.ppm = strtod(optarg, &endp);
            if ((optarg == endp) || (*endp != '\0'))
            {
                llog.logE("Error: Could not parse ppm value.");
                return false;
            }
            break;
        case 's':
            // Self-Cal
            config.self_cal = true;
            break;
        case 'f':
            // Free Running
            config.self_cal = false;
            break;
        case 'r':
            // Repeat
            config.repeat = true;
            break;
        case 'x':
            // Terminate after X
            config.terminate = strtol(optarg, &endp, 10);
            if ((optarg == endp) || (*endp != '\0'))
            {
                llog.logE("Error: Could not parse termination argument.");
                return false;
            }
            if (config.terminate < 1)
            {
                llog.logE("Error: Termination parameter must be >= 1.");
                return false;
            }
            break;
        case 'o':
            // Use random offset
            config.random_offset = true;
            break;
        case 't':
            // Set a test tone
            config.test_tone = strtod(optarg, &endp);
            config.mode = TONE;
            if ((optarg == endp) || (*endp != '\0'))
            {
                llog.logE("Error: could not parse test tone frequency.");
                return false;
            }
            break;
        case 'i':
            // Use INI file
            config.inifile = optarg;
            config.useini = true;
            break;
        case 'n':
            // No delay, transmit immediately
            config.no_delay = true;
            break;
        case 'l':
            // Use LED
            config.use_led = true;
            break;
        case 'D':
            // Daemon mode, repeats indefinitely
            config.daemon_mode = true;
            config.repeat = true; // Repeat must be true in a daemon setup
            llog.setDaemon(config.daemon_mode);
            break;
        case 'd':
            {
                // Set power output 0-7
                int _pwr = strtol(optarg, NULL, 10);
                if (_pwr < 0 || _pwr > 7 )
                {
                    config.power_level = 7;
                }
                else
                {
                    config.power_level = _pwr;
                }
                config.power_level = strtol(optarg, NULL, 10);
                break;
            }
        default:
            return false;
        }
    }
    if (config.useini == false) { config.xmit_enabled = true; }
    return true;
}

bool parseConfigData(const int &argc, char *const argv[], bool reparse = false)
{
    if (config.useini)
    {
        if (! reparse) iniMonitor.filemon(config.inifile.c_str());
        if ( !getINIValues(reparse) )
        {
            llog.logE("Error: Failed to reload the INI.");
            exit(-1);
        }
        config.center_freq_set.clear();

        std::vector<std::string> freq_list;
        std::istringstream s ( config.frequency_string );
        freq_list.insert(freq_list.end(), std::istream_iterator<std::string>(s), std::istream_iterator<std::string>());

        for (std::vector<std::string>::iterator f=freq_list.begin(); f!=freq_list.end(); ++f) 
        {
            std::string fString{ *f };
            const char * fs = fString.c_str();
            double parsed_freq;
            convertToFreq(fs, parsed_freq);
            config.center_freq_set.push_back(parsed_freq);
        }
    }
    else
    {
        if ( ! reparse)
        {
            unsigned int n_free_args = 0;
            while (optind < argc)
            {
                // Check for callsign, grid_square, tx_power
                if (n_free_args == 0)
                {
                    config.callsign = argv[optind++];
                    n_free_args++;
                    continue;
                }
                if (n_free_args == 1)
                {
                    config.grid_square = argv[optind++];
                    n_free_args++;
                    continue;
                }
                if (n_free_args == 2)
                {
                    config.tx_power = argv[optind++];
                    n_free_args++;
                    continue;
                }
                // Must be a frequency
                // First see if it is a string.
                double parsed_freq;
                const char * argument = argv[optind];
                convertToFreq(argument, parsed_freq);
                optind++;
                config.center_freq_set.push_back(parsed_freq);
            }
        }
    }

    // Convert to uppercase
    transform(config.callsign.begin(), config.callsign.end(), config.callsign.begin(), ::toupper);
    transform(config.grid_square.begin(), config.grid_square.end(), config.grid_square.begin(), ::toupper);

    // Check consistency among command line options.
    if (config.ppm && config.self_cal)
    {
        llog.logE("Warning: ppm value is being ignored.");
        config.ppm = 0.0;
    }
    if (config.mode == TONE)
    {
        if ((config.callsign != "") || (config.grid_square != "") || (config.tx_power != "") || (config.center_freq_set.size() != 0) || config.random_offset)
        {
            llog.logE("Warning: Callsign, gridsquare, etc. are ignored when generating test tone.");
        }
        config.random_offset = 0;
        if (config.test_tone <= 0)
        {
            llog.logE("Error: Test tone frequency must be positive.");
            exit(-1);
        }
    }
    else
    {
        if ((config.callsign == "") || (config.grid_square == "") || (config.tx_power == "") || (config.center_freq_set.size() == 0))
        {
            llog.logE("Error: must specify callsign, gridsquare, dBm, and at least one frequency.");
            llog.logE("Try: wspr --help");
            exit(-1);
        }
    }

    // Print a summary of the parsed options
    if (config.mode == WSPR)
    {
        llog.logS("WSPR packet payload:");
        llog.logS("- Callsign: ", config.callsign);
        llog.logS("- Locator:  ", config.grid_square);
        llog.logS("- Power:    ", config.tx_power, " dBm");
        llog.logS("Requested TX frequencies:");
        std::stringstream temp;
        for (unsigned int t = 0; t < config.center_freq_set.size(); t++)
        {
            temp << std::setprecision(6) << std::fixed;
            temp << "- " << config.center_freq_set[t] / 1e6 << " MHz" << std::endl;
        }
        llog.logS(temp.str());
        temp.str("");
        if (config.self_cal)
        {
            temp << "- Using NTP to calibrate transmission frequency." << std::endl;
        }
        else if (config.ppm)
        {
            temp << "- PPM value to be used for all transmissions: " << config.ppm << std::endl;
        }
        if (config.terminate > 0)
        {
            temp << "- TX will stop after " << config.terminate << " transmissions." << std::endl;
        }
        else if (config.repeat && ! config.daemon_mode)
        {
            temp << "- Transmissions will continue forever until stopped with CTRL-C." << std::endl;
        }
        if (config.random_offset)
        {
            temp << "- A small random frequency offset will be added to all transmissions." << std::endl;
        }
        if (temp.str().length())
        {
            llog.logS("Extra options:");
            llog.logS(temp.str());
        }
    }
    else
    {
        std::stringstream temp;
        temp << std::setprecision(6) << std::fixed << "A test tone will be generated at frequency " << config.test_tone / 1e6 << " MHz.";
        llog.logS(temp.str());
        if (config.self_cal)
        {
            llog.logS("NTP will be used to calibrate the tone frequency.");
        }
        else if (config.ppm)
        {
            llog.logS("PPM value to be used to generate the tone: ", config.ppm);
        }
    }
    return true;
}

bool parseConfigData(bool reparse)
{
    return parseConfigData(0, {}, reparse);
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
            while (iniMonitor.changed()) {;;}

            llog.logS("Notice: INI file changed, reloading parameters.");
            parseConfigData(true);
            return false; // Need to reload
        }
        time(&t);
        ptm = gmtime(&t);
        if ((ptm->tm_min % minute) == 0 && ptm->tm_sec == 0)
            break;
        usleep(1000);
    }
    usleep(1000000); // Wait another second
    return true; // OK to proceed
}

void update_ppm()
{
    // Call ntp_adjtime() to obtain the latest calibration coefficient.

    struct timex ntx;
    int status;
    double ppm_new;

    ntx.modes = 0; /* only read */
    status = ntp_adjtime(&ntx);

    if (status != TIME_OK)
    {
        // cerr << "Error: clock not synchronized" << std::endl;
        // return;
    }

    ppm_new = (double)ntx.freq / (double)(1 << 16); /* frequency scale */
    if (abs(ppm_new) > 200)
    {
        llog.logE("Warning: Absolute ppm value is greater than 200 and is being ignored.");
    }
    else
    {
        if (config.ppm != ppm_new)
        {
            llog.logS("Obtained new ppm value: ", ppm_new);
        }
        config.ppm = ppm_new;
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
        llog.logE("Failed to open mailbox.");
        exit(-1);
    }
}

void cleanup()
{
    // Called when exiting or when a signal is received.
    pinLow(LED_PIN);
    disable_clock();
    unSetupDMA();
    deallocMemPool();
    unlink(LOCAL_DEVICE_FILE_NAME);
}

void cleanupAndExit(int sig)
{
    // Called when a signal is received. Automatically calls cleanup().
    llog.logE("Exiting, caught signal: ", sig);
    cleanup();
    exit(-1);
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
        llog.logE("Warning: pthread_setschedparam (increase thread priority) returned non-zero: ", ret);
    }
}

void setup_peri_base_virt(volatile unsigned *&peri_base_virt)
{
    // Create the memory map between virtual memory and the peripheral range
    // of physical memory.

    int mem_fd;
    unsigned gpio_base = gpioBase();
    // open /dev/mem
    if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
    {
        llog.logE("Error: Can't open /dev/mem");
        exit(-1);
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
        llog.logE("Error: peri_base_virt mmap error.");
        exit(-1);
    }
    close(mem_fd);
}

int main(const int argc, char *const argv[])
{
    if ( ! parse_commandline(argc, argv) ) return 1;
    llog.logS("Wsprry Pi v", exeversion(), " (", branch(), ").");
    llog.logS("Running on: ", RPiVersion(), ".");
    getPLLD(); // Get PLLD Frequency
    setupGPIO(LED_PIN);

    if ( ! parseConfigData(argc, argv) ) return 1;

    // Make sure we're the only wspr process
    wspr::SingletonProcess singleton(SINGLETON_PORT);
    try
    {
        if (!singleton())
        {
            llog.logE("Process already running; see ", singleton.GetLockFileName());
            return 1;
        }
    }
    catch (const std::exception& e)
    {
        llog.logE("Failed to enforce singleton: ", e.what());
        return 1;
}

    // Catch all signals (like ctrl+c, ctrl+z, ...) to ensure DMA is disabled
    for (int i = 0; i < 64; i++)
    {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = cleanupAndExit;
        sigaction(i, &sa, NULL);
    }
    atexit(cleanup);
    setSchedPriority(30);

    // Initialize the RNG
    srand(time(NULL));

    int nbands = config.center_freq_set.size();

    // Initial configuration
    struct PageInfo constPage;
    struct PageInfo instrPage;
    struct PageInfo instrs[1024];
    setup_peri_base_virt(peri_base_virt);

    // Set up DMA
    open_mbox();
    txon(false, 0);
    setupDMA(constPage, instrPage, instrs);
    txoff();

    if (config.mode == TONE)
    {
        // Test tone mode...
        double wspr_symtime = WSPR_SYMTIME;
        double tone_spacing = 1.0 / wspr_symtime;

        std::stringstream temp;
        temp << std::setprecision(6) << std::fixed << "Transmitting test tone on frequency " << config.test_tone / 1.0e6 << " MHz.";
        llog.logS(temp.str());
        llog.logS("Press CTRL-C to exit.");

        txon(config.use_led, config.power_level);
        int bufPtr = 0;
        std::vector<double> dma_table_freq;
        // Set to non-zero value to ensure setupDMATab is called at least once.
        double ppm_prev = 123456;
        double center_freq_actual;
        while (true)
        {
            if (config.self_cal)
            {
                update_ppm();
            }
            if (config.ppm != ppm_prev)
            {
                setupDMATab(config.test_tone + 1.5 * tone_spacing, tone_spacing, config.f_plld_clk * (1 - config.ppm / 1e6), dma_table_freq, center_freq_actual, constPage);
                // cout << std::setprecision(30) << dma_table_freq[0] << std::endl;
                // cout << std::setprecision(30) << dma_table_freq[1] << std::endl;
                // cout << std::setprecision(30) << dma_table_freq[2] << std::endl;
                // cout << std::setprecision(30) << dma_table_freq[3] << std::endl;
                if (center_freq_actual != config.test_tone + 1.5 * tone_spacing)
                {
                    std::stringstream temp;
                    temp << std::setprecision(6) << std::fixed << "Warning: Test tone will be transmitted on " << (center_freq_actual - 1.5 * tone_spacing) / 1e6 << " MHz due to hardware limitations." << std::endl;
                    llog.logE(temp.str());
                }
                ppm_prev = config.ppm;
            }
            txSym(0, center_freq_actual, tone_spacing, 60, dma_table_freq, F_PWM_CLK_INIT, instrs, constPage, bufPtr);
        }
    }
    else
    {
        // WSPR mode
        for (;;)
        { // Reload Loop >
            // Initialize WSPR Message (message)
            WsprMessage message(
                const_cast<char *>(config.callsign.c_str()),
                const_cast<char *>(config.grid_square.c_str()),
                std::stoi(config.tx_power)
            );

            // Access the generated symbols
            unsigned char* symbols = message.symbols;

#ifdef WSPR_DEBUG
            // Print encoded packet
            std::cout << "WSPR codeblock:";
            std::cout << std::endl;
            for (int i = 0; i < WsprMessage::size; i++) {
                if (i) std::cout << ",";
                std::cout << static_cast<int>(symbols[i]);
            }
            std::cout << std::endl;
#endif

            llog.logS("Ready to transmit (setup complete).");
            int band = 0;
            int n_tx = 0;
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
                if ((center_freq_desired != 0) && config.random_offset)
                {
                    center_freq_desired += (2.0 * rand() / ((double)RAND_MAX + 1.0) - 1.0) * (wspr15 ? WSPR15_RAND_OFFSET : WSPR_RAND_OFFSET);
                }

                // Status message before transmission
                std::stringstream temp;
                temp << std::setprecision(6) << std::fixed;
                temp << "Center frequency for " << (wspr15 ? "WSPR-15" : "WSPR") << " trans: " << center_freq_desired / 1e6 << " MHz.";
                llog.logS(temp.str());

                // Wait for WSPR transmission window to arrive.
                if (config.no_delay)
                {
                    llog.logS("Transmitting immediately (not waiting for WSPR window.)");
                }
                else
                {
                    llog.logS("Waiting for next WSPR transmission window.");
                    if ( ! wait_every((wspr15) ? 15 : 2) )
                    {
                        // Break and reload if ini changes
                        break;
                    };
                }

                // Update crystal calibration information
                if (config.self_cal)
                {
                    update_ppm();
                }

                // Create the DMA table for this center frequency
                std::vector<double> dma_table_freq;
                double center_freq_actual;
                if (center_freq_desired)
                {
                    setupDMATab(center_freq_desired, tone_spacing, config.f_plld_clk * (1 - config.ppm / 1e6), dma_table_freq, center_freq_actual, constPage);
                }
                else
                {
                    center_freq_actual = center_freq_desired;
                }

                // Send the message if freq != 0 and transmission is enabled
                if (center_freq_actual && config.xmit_enabled)
                {
                    // Print a status message right before transmission begins.
                    llog.logS("Transmission started.");

                    struct timeval tvBegin, sym_start, diff;
                    gettimeofday(&tvBegin, NULL);
                    int bufPtr = 0;

                    // Get Begin Timestamp
                    auto txBegin = std::chrono::high_resolution_clock::now();

                    txon(config.use_led, config.power_level);
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
                    n_tx++;

                    // Turn transmitter off
                    txoff(config.use_led);

                    // Get End Timestamp
                    auto txEnd = std::chrono::high_resolution_clock::now();
                    // Calculate duration in <double> seconds
                    std::chrono::duration<double, std::milli> elapsed = (txEnd - txBegin) / 1000;
                    double num_seconds = elapsed.count();
                    llog.logS("Transmission completed, (", num_seconds, " sec.)");
                }
                else
                {
                    llog.logS("Skipping transmission.");
                    usleep(1000000);
                }
                
                // Advance to next band
                band = (band + 1) % nbands;
                if ((band == 0) && !config.repeat)
                {
                    return 0;
                }
                if ((config.terminate > 0) && (n_tx >= config.terminate))
                {
                    return 0;
                }
            }
        } // < Reload Loop
    }

    return 0;
}
