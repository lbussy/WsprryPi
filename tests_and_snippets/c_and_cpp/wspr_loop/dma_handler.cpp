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
#include <chrono>
#include <ctime>
#include <cstring>
#include <thread>

#include "lcblog.hpp"
#include "wspr_message.hpp"
#include "version.hpp"
#include "constants.hpp"
#include "hardware_access.hpp"

#include "main.hpp"

#ifdef _TEST
constexpr const std::string_view MODE = "TONE";
constexpr const double TEST_TONE = 780000.0;
constexpr const double WSPR_FREQUENCY = 7040100.0;
constexpr const double PPM_VAL = 12.0;
constexpr const char *CALL = "AA0NT";
constexpr const char *GRID = "EM18";
constexpr const int PWR = 20;
static struct termios original_tty; // TODO:  Temporary for signal handling
static bool tty_saved = false;
volatile sig_atomic_t running = 1;
#endif // _TEST

/**
 * @brief Global configuration instance for argument parsing.
 *
 * This global instance of `ArgParserConfig` holds the parsed
 * command-line arguments and configuration settings used throughout
 * the application. It is defined in `arg_parser.cpp` and declared
 * as `extern` in `arg_parser.hpp` so it can be accessed globally.
 *
 * @note Ensure that `arg_parser.hpp` is included in any file that
 *       needs access to this configuration instance.
 *
 * @see ArgParserConfig
 */
struct ArgParserConfig
{
    double f_plld_clk; ///< Phase-Locked Loop D clock, default is 500 MHz
                       // Output of: vcgencmd measure_clock plld
    int mem_flag;      ///< TODO:  Define this - Placeholder for memory management flags.
    /**
     * @brief Default constructor initializing all configuration parameters.
     *
     * Initializes the structure with default values:
     * - `f_plld_clk = 0.0`
     * - `mem_flag = 0`
     */
    ArgParserConfig() : f_plld_clk(0.0),
                        mem_flag(0)
    {
    }
};

// TODO: declaration for global configuration object.
ArgParserConfig config;

// Logging library
LCBLog llog;

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

// Convert from a bus address to a physical address.
#define BUS_TO_PHYS(x) ((x) & ~0xC0000000)

// peri_base_virt is the base virtual address that a userspace program (this
// program) can use to read/write to the the physical addresses controlling
// the peripherals. This address is mapped at runtime using mmap and /dev/mem.
// This must be declared global so that it can be called by the atexit
// function.
volatile unsigned *peri_base_virt = NULL;

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
    switch (7) // TODO:  Un-hardcode this
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
    uint32_t value;
    std::memcpy(&value, &setupword, sizeof(value));
    ACCESS_BUS_ADDR(CM_GP0CTL_BUS) = value;
}

void txoff()
{
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

int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1)
{
    // Return 1 if the difference is negative, otherwise 0.
    long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
    result->tv_sec = diff / 1000000;
    result->tv_usec = diff % 1000000;

    return (diff < 0);
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

// TODO:  Delete this
void restore_terminal_signals()
{
    // Restore terminal attributes if they were previously saved.
    if (tty_saved)
    {
        if (tcsetattr(STDIN_FILENO, TCSANOW, &original_tty) != 0)
        {
            llog.logE(ERROR, "Failed to restore terminal settings: ", std::strerror(errno));
        }
        else
        {
            llog.logS(DEBUG, "Terminal signals restored to original state.");
        }
    }
    else
    {
        llog.logS(WARN, "Terminal settings were not saved. Nothing to restore.");
    }
}

void exit_and_cleanup()
{
    restore_terminal_signals();
    disable_clock();
    unSetupDMA();
    deallocMemPool();
    unlink(LOCAL_DEVICE_FILE_NAME);
    std::exit(EXIT_SUCCESS);
}

// TODO:  Delete this
void suppress_terminal_signals()
{
    // Save current terminal settings if possible.
    if (tcgetattr(STDIN_FILENO, &original_tty) == 0)
    {
        tty_saved = true;
        struct termios tty = original_tty;

        // Disable input echo while keeping signals enabled.
        tty.c_lflag &= ~ECHO;

        if (tcsetattr(STDIN_FILENO, TCSANOW, &tty) != 0)
        {
            llog.logE(ERROR, "Failed to suppress terminal signals: ", std::strerror(errno));
        }
        else
        {
            llog.logS(DEBUG, "Terminal signals suppressed (input echo disabled).");
        }
    }
    else
    {
        llog.logE(ERROR, "Failed to retrieve terminal settings: ", std::strerror(errno));
    }
}

// TODO:  Delete this
std::string signal_to_string(int signum)
{
    // Map of signal numbers to their corresponding string representations.
    static const std::unordered_map<int, std::string> signal_map = {
        {SIGINT, "SIGINT"},
        {SIGTERM, "SIGTERM"},
        {SIGQUIT, "SIGQUIT"},
        {SIGSEGV, "SIGSEGV"},
        {SIGBUS, "SIGBUS"},
        {SIGFPE, "SIGFPE"},
        {SIGILL, "SIGILL"},
        {SIGHUP, "SIGHUP"},
        {SIGABRT, "SIGABRT"}};

    // Find the signal in the map and return its string representation.
    auto it = signal_map.find(signum);
    return (it != signal_map.end()) ? it->second : "UNKNOWN_SIGNAL";
}

// TODO:  Delete this
void signal_handler(int signum)
{
    llog.logS(DEBUG, "Signal caught:", signum);
    running = 0; // Stop main loop
    // Convert signal number to a human-readable string.
    std::string signal_name = signal_to_string(signum);
    std::ostringstream oss;
    oss << "Received " << signal_name << ". Shutting down.";
    std::string log_message = oss.str();

    // Handle the signal based on type.
    switch (signum)
    {
    // Fatal signals that require immediate exit.
    case SIGSEGV:
    case SIGBUS:
    case SIGFPE:
    case SIGILL:
    case SIGABRT:
        llog.logE(FATAL, log_message);
        exit_and_cleanup();
        restore_terminal_signals();
        std::quick_exit(signum); // Immediate exit without cleanup.
        break;

    // Graceful shutdown signals.
    case SIGINT:
    case SIGTERM:
    case SIGQUIT:
    case SIGHUP:
        llog.logS(INFO, log_message);
        exit_and_cleanup();
        restore_terminal_signals();
        break;

    // Unknown signals are treated as fatal.
    default:
        llog.logE(FATAL, "Unknown signal caught. Exiting.");
        break;
    }
}

// TODO:  Delete this
void register_signal_handlers()
{
    // Block signals in the main thread to prevent default handling.
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // Register each signal
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);

    llog.logS(DEBUG, "Signal handlers installed.");

    // Suppress terminal signals to avoid disruption during execution.
    suppress_terminal_signals();

    llog.logS(DEBUG, "Signal handling thread started.");
}

int main(const int argc, char *const argv[])
{
    llog.setLogLevel(DEBUG);

    llog.logS(INFO, version_string());
    llog.logS(INFO, "Running on:", getRaspberryPiModel(), ".");

    // TODO: Temporary
    register_signal_handlers();

    getPLLD(); // Get PLLD Frequency

    setSchedPriority(30);

    // Initialize the RNG
    srand(time(NULL));

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

    // Test tone mode...
    double wspr_symtime = WSPR_SYMTIME;
    double tone_spacing = 1.0 / wspr_symtime;

    {
        std::stringstream temp;
        temp << std::setprecision(6) << std::fixed << "Transmitting test tone on frequency " << TEST_TONE / 1.0e6 << " MHz.";
        llog.logS(INFO, temp.str());
        llog.logS(INFO, "Press CTRL-C to exit.");
    }

    txon();
    int bufPtr = 0;
    std::vector<double> dma_table_freq;
    // Set PPM_PREV to non-zero value to ensure setupDMATab is called at least once.
    double center_freq_actual;
    while (running)
    {
        if (PPM_VAL == PPM_VAL)
        {
            setupDMATab(TEST_TONE + 1.5 * tone_spacing, tone_spacing, config.f_plld_clk * (1 - PPM_VAL / 1e6), dma_table_freq, center_freq_actual, constPage);
            // cout << std::setprecision(30) << dma_table_freq[0] << std::endl;
            // cout << std::setprecision(30) << dma_table_freq[1] << std::endl;
            // cout << std::setprecision(30) << dma_table_freq[2] << std::endl;
            // cout << std::setprecision(30) << dma_table_freq[3] << std::endl;
            if (center_freq_actual != TEST_TONE + 1.5 * tone_spacing)
            {
                std::stringstream temp;
                temp << std::setprecision(6) << std::fixed << "Test tone will be transmitted on " << (center_freq_actual - 1.5 * tone_spacing) / 1e6 << " MHz due to hardware limitations." << std::endl;
                llog.logE(INFO, temp.str());
            }
        }
        txSym(0, center_freq_actual, tone_spacing, 60, dma_table_freq, F_PWM_CLK_INIT, instrs, constPage, bufPtr);
    }

    exit_and_cleanup();
    std::exit(EXIT_SUCCESS);
}
