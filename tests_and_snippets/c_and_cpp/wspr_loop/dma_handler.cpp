#include "dma_handler.hpp"

#include "arg_parser.hpp"
#include "constants.hpp"
#include "ini_file.hpp"
#include "logging.hpp"
#include "monitorfile.hpp"
#include "singleton.hpp"
#include "transmit.hpp"
#include "wspr_message.hpp"
#include "version.hpp"
// #include "hardware_access.hpp"

#include <algorithm>
#include <vector>
#include <iterator>
#include <stdexcept>
#include <cctype>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <iostream>
#include <chrono>
#include <ctime>
#include <cstring>
#include <thread>

#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <termios.h>
#include <unistd.h>

double wspr_symtime;
double tone_spacing;
std::vector<double> dma_frequency_table;

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

struct PageInfo
{
    // Virtual and bus addresses of a page of physical memory.
    void *b; // bus address
    void *v; // virtual address
};

struct PageInfo constantsPage;
struct PageInfo instructionsPage;
struct PageInfo instructions[1024];

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
    const std::vector<double> &dma_frequency_table,
    const double &f_pwm_clk,
    struct PageInfo instructions[],
    struct PageInfo &constantsPage,
    int &bufPtr)
{
    // Transmit symbol sym for tsym seconds.
    //
    // Note:
    // Upon entering this function at the beginning of a WSPR transmission, we
    // do not know which DMA table entry is being processed by the DMA engine.
    const int f0_idx = sym_num * 2;
    const int f1_idx = f0_idx + 1;
    const double f0_freq = dma_frequency_table[f0_idx];
    const double f1_freq = dma_frequency_table[f1_idx];
    const double tone_freq = center_freq - 1.5 * tone_spacing + sym_num * tone_spacing;
    // Double check...
    assert((tone_freq >= f0_freq) && (tone_freq <= f1_freq));
    const double f0_ratio = 1.0 - (tone_freq - f0_freq) / (f1_freq - f0_freq);
    // cout << "f0_ratio = " << f0_ratio << std::endl;
    assert((f0_ratio >= 0) && (f0_ratio <= 1));
    const long int n_pwmclk_per_sym = round(f_pwm_clk * tsym);

    long int n_pwmclk_transmitted = 0;
    long int n_f0_transmitted = 0;
    // printf("<instructions[bufPtr] begin=%x>",(unsigned)&instructions[bufPtr]);
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
        while (ACCESS_BUS_ADDR(DMA_BUS_BASE + 0x04 /* CurBlock*/) == (long int)(instructions[bufPtr].b))
            usleep(100);
        ((struct CB *)(instructions[bufPtr].v))->SOURCE_AD = (long int)constantsPage.b + f0_idx * 4;

        // Wait for n_f0 PWM clocks
        bufPtr++;
        while (ACCESS_BUS_ADDR(DMA_BUS_BASE + 0x04 /* CurBlock*/) == (long int)(instructions[bufPtr].b))
            usleep(100);
        ((struct CB *)(instructions[bufPtr].v))->TXFR_LEN = n_f0;

        // Set GPIO pin to transmit f1
        bufPtr++;
        while (ACCESS_BUS_ADDR(DMA_BUS_BASE + 0x04 /* CurBlock*/) == (long int)(instructions[bufPtr].b))
            usleep(100);
        ((struct CB *)(instructions[bufPtr].v))->SOURCE_AD = (long int)constantsPage.b + f1_idx * 4;

        // Wait for n_f1 PWM clocks
        bufPtr = (bufPtr + 1) % (1024);
        while (ACCESS_BUS_ADDR(DMA_BUS_BASE + 0x04 /* CurBlock*/) == (long int)(instructions[bufPtr].b))
            usleep(100);
        ((struct CB *)(instructions[bufPtr].v))->TXFR_LEN = n_f1;

        // Update counters
        n_pwmclk_transmitted += n_pwmclk;
        n_f0_transmitted += n_f0;
    }
    // printf("<instructions[bufPtr]=%x %x>",(unsigned)instructions[bufPtr].v,(unsigned)instructions[bufPtr].b);
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

void set_dma_tone_table(
    const double &center_freq_desired,
    const double &tone_spacing,
    const double &plld_actual_freq,
    std::vector<double> &dma_frequency_table,
    double &center_freq_actual,
    struct PageInfo &constantsPage)
{
    // Make sure that all the WSPR tones can be produced solely by varying the fractional
    // part of the frequency divider (second 12 bits.)
    //
    // Each tuning word is 32-bits, first 8 bits are the "magic word" 0x5a (90 in decimal).
    // The next 12 bits after that, the integer part, is the coarse tuning and must be
    // consistent across all words to prevent phase noise when switching, this section tweaks
    // the requested (center_freq_desired) frequency freq up/down to fit that and returns the
    // result in center_freq_actual.
    //
    // An integer part change, or a "coarse tuning word", is a 4.5 kHz step, where the fine
    // tuning word is approximately 1.1 Hz.
    //
    // The max potential change applied here is 2 x tone spacing with four words, so the
    // rewquested frequencyncould change +- (2 * 1.46 Hz) or 2.92 Hz.
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
    dma_frequency_table.resize(1024);
    for (int i = 0; i < 1024; i++)
    {
        dma_frequency_table[i] = plld_actual_freq / (tuning_word[i] / pow(2.0, 12));
        ((int *)(constantsPage.v))[i] = (0x5a << 24) + tuning_word[i];
        if ((i % 2 == 0) && (i < 8))
        {
            assert((tuning_word[i] & (~0xfff)) == (tuning_word[i + 1] & (~0xfff)));
        }
    }
}

void setup_dma_instructions(
    struct PageInfo &constantsPage,
    struct PageInfo &instructionsPage,
    struct PageInfo instructions[])
{
    // Create the memory structures needed by the DMA engine and perform initial
    // clock configuration.

    allocMemPool(1025);

    // Allocate a page of ram for the constants
    getRealMemPageFromPool(&constantsPage.v, &constantsPage.b);

    // Create 1024 instructions allocating one page at a time.
    // Even instructions target the GP0 Clock divider
    // Odd instructions target the PWM FIFO
    int instrCnt = 0;
    while (instrCnt < 1024)
    {
        // Allocate a page of ram for the instructions
        getRealMemPageFromPool(&instructionsPage.v, &instructionsPage.b);

        // Make copy instructions
        // Only create as many instructions as will fit in the recently
        // allocated page. If not enough space for all instructions, the
        // next loop will allocate another page.
        struct CB *instr0 = (struct CB *)instructionsPage.v;
        int i;
        for (i = 0; i < (signed)(4096 / sizeof(struct CB)); i++)
        {
            instructions[instrCnt].v = (void *)((long int)instructionsPage.v + sizeof(struct CB) * i);
            instructions[instrCnt].b = (void *)((long int)instructionsPage.b + sizeof(struct CB) * i);
            instr0->SOURCE_AD = (unsigned long int)constantsPage.b + 2048;
            instr0->DEST_AD = PWM_BUS_BASE + 0x18 /* FIF1 */;
            instr0->TXFR_LEN = 4;
            instr0->STRIDE = 0;
            // instr0->NEXTCONBK = (int)instructionsPage.b + sizeof(struct CB)*(i+1);
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
                ((struct CB *)(instructions[instrCnt - 1].v))->NEXTCONBK = (long int)instructions[instrCnt].b;
            instr0++;
            instrCnt++;
        }
    }
    // Create a circular linked list of instructions
    ((struct CB *)(instructions[1023].v))->NEXTCONBK = (long int)instructions[0].b;

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
    DMA0->CONBLK_AD = (unsigned long int)(instructionsPage.b);
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

void set_scheduling_priority(int priority)
{
    // TODO:  We have this in scheduling.*

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

void cleanup_dma_registers()
{
    disable_clock();
    unSetupDMA();
    deallocMemPool();
    unlink(LOCAL_DEVICE_FILE_NAME);
}

void dma_prep()
{
    getPLLD(); // Get PLLD Frequency

    set_scheduling_priority(30);

    // Initialize the RNG
    srand(time(NULL));

    // Initial configuration
    setup_peri_base_virt(peri_base_virt);

    // Set up DMA
    open_mbox();
    txon();
    setup_dma_instructions(constantsPage, instructionsPage, instructions);
    txoff();
}

double dma_setup(double center_freq_desired)
{
    // TODO: Do this once per frequency or PPM change
    // Determine if this is a WSSPR15 request
    bool wspr15 =
        (center_freq_desired > 137600 && center_freq_desired < 137625) ||
        (center_freq_desired > 475800 && center_freq_desired < 475825) ||
        (center_freq_desired > 1838200 && center_freq_desired < 1838225);

    // Setup symbol time and spacing for WSPR type
    wspr_symtime = (wspr15) ? 8.0 * WSPR_SYMTIME : WSPR_SYMTIME;
    tone_spacing = 1.0 / wspr_symtime;

    // Add random offset (+-)
    if (ini.get_bool_value("Extended", "Offset"))
    {
        center_freq_desired += (2.0 * rand() / ((double)RAND_MAX + 1.0) - 1.0) * (wspr15 ? WSPR15_RAND_OFFSET : WSPR_RAND_OFFSET);
    }

    // Status message before transmission
    {
        std::stringstream temp;
        temp << std::setprecision(6) << std::fixed;
        temp << "Center frequency set for " << (wspr15 ? "WSPR-15" : "WSPR") << " trans: " << center_freq_desired / 1e6 << " MHz.";
        llog.logS(INFO, temp.str());
    }

    // Create the DMA table for this center frequency
    double center_freq_actual; // This may be modified by reference in set_dma_tone_table()
    set_dma_tone_table(center_freq_desired, tone_spacing, config.f_plld_clk * (1 - ini.get_double_value("Extended", "PPM") / 1e6), dma_frequency_table, center_freq_actual, constantsPage);
    return center_freq_actual;
}

void tx_wspr(WsprMessage message, double center_freq_actual)
{
    // Access the generated symbols
    unsigned char *symbols = message.symbols;

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
        txSym(symbols[i], center_freq_actual, tone_spacing, sched_end - elapsed, dma_frequency_table, F_PWM_CLK_INIT, instructions, constantsPage, bufPtr);
    }

    // Turn transmitter off
    txoff();

    // Get End Timestamp
    auto txEnd = std::chrono::high_resolution_clock::now();
    // Calculate duration in <double> seconds
    std::chrono::duration<double, std::milli> elapsed = (txEnd - txBegin) / 1000;
    double num_seconds = elapsed.count();
    llog.logS(INFO, "Transmission complete (", num_seconds, " sec.)");
}

void tx_tone(double tone_frequency)
{
    // Test tone mode...
    double wspr_symtime = WSPR_SYMTIME;
    double tone_spacing = 1.0 / wspr_symtime;

    std::stringstream temp;
    temp << std::setprecision(6) << std::fixed << "Transmitting test tone on frequency " << tone_frequency / 1.0e6 << " MHz.";
    llog.logS(INFO, temp.str());
    llog.logS(INFO, "Press CTRL-C to exit.");

    txon();
    int bufPtr = 0;
    std::vector<double> dma_frequency_table;
    double center_freq_actual;
    while (true)
    {
        if (true) // TODO:  Run this at least once and after every PPM update
        {
            set_dma_tone_table(tone_frequency + 1.5 * tone_spacing, tone_spacing, config.f_plld_clk * (1 - ini.get_double_value("Extended", "PPM") / 1e6), dma_frequency_table, center_freq_actual, constantsPage);
            // cout << std::setprecision(30) << dma_frequency_table[0] << std::endl;
            // cout << std::setprecision(30) << dma_frequency_table[1] << std::endl;
            // cout << std::setprecision(30) << dma_frequency_table[2] << std::endl;
            // cout << std::setprecision(30) << dma_frequency_table[3] << std::endl;
            if (center_freq_actual != tone_frequency + 1.5 * tone_spacing)
            {
                std::stringstream temp;
                temp << std::setprecision(6) << std::fixed << "Test tone will be transmitted on " << (center_freq_actual - 1.5 * tone_spacing) / 1e6 << " MHz due to hardware limitations." << std::endl;
                llog.logE(INFO, temp.str());
            }
        }
        txSym(0, center_freq_actual, tone_spacing, 60, dma_frequency_table, F_PWM_CLK_INIT, instructions, constantsPage, bufPtr);
    }
}
