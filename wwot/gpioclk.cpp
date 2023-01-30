// Simple program to route either the cyrstal clock or PLL clock to the
// output GPIO pin.

/*
License:
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <malloc.h>
#include <time.h>
#include <sys/time.h>
#include <getopt.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <bcm_host.h>

using namespace std;

static int ver = bcm_host_get_processor_id();

// Used for debugging
#define MARK std::cout << "Currently in file: " << __FILE__ << " line: " << __LINE__ << std::endl

#define PAGE_SIZE (4 * 1024)
#define BLOCK_SIZE (4 * 1024)

// This must be declared global so that it can be called by the atexit
// function.
volatile unsigned *allof7e = NULL;

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio + ((g) / 10)) &= ~(7 << (((g) % 10) * 3))
#define OUT_GPIO(g) *(gpio + ((g) / 10)) |= (1 << (((g) % 10) * 3))
#define SET_GPIO_ALT(g, a) *(gpio + (((g) / 10))) |= (((a) <= 3 ? (a) + 4 : (a) == 4 ? 3  \
                                                                                     : 2) \
                                                      << (((g) % 10) * 3))

#define GPIO_SET *(gpio + 7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio + 10) // clears bits which are 1 ignores bits which are 0
#define GPIO_GET *(gpio + 13) // sets   bits which are 1 ignores bits which are 0

#define ACCESS(base) *(volatile int *)((long int)allof7e + base - 0x7e000000)
#define SETBIT(base, bit) ACCESS(base) |= 1 << bit
#define CLRBIT(base, bit) ACCESS(base) &= ~(1 << bit)
#define CM_GP0CTL (0x7e101070)
#define GPFSEL0 (0x7E200000)
#define PADS_GPIO_0_27 (0x7e10002c)
#define CM_GP0DIV (0x7e101074)
#define CLKBASE (0x7E101000)
#define DMABASE (0x7E007000)
#define PWMBASE (0x7e20C000) /* PWM controller */

typedef enum
{
    PLLD,
    XTAL
} source_t;

struct GPCTL
{
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

void txon(
    const source_t &source,
    const double &divisor)
{
    SETBIT(GPFSEL0, 14);
    CLRBIT(GPFSEL0, 13);
    CLRBIT(GPFSEL0, 12);

    // Set GPIO drive strength, more info: http://www.scribd.com/doc/101830961/GPIO-Pads-Control2
    // ACCESS(PADS_GPIO_0_27) = 0x5a000018 + 0;  //2mA -3.4dBm
    // ACCESS(PADS_GPIO_0_27) = 0x5a000018 + 1;  //4mA +2.1dBm
    // ACCESS(PADS_GPIO_0_27) = 0x5a000018 + 2;  //6mA +4.9dBm
    // ACCESS(PADS_GPIO_0_27) = 0x5a000018 + 3;  //8mA +6.6dBm(default)
    // ACCESS(PADS_GPIO_0_27) = 0x5a000018 + 4;  //10mA +8.2dBm
    // ACCESS(PADS_GPIO_0_27) = 0x5a000018 + 5;  //12mA +9.2dBm
    // ACCESS(PADS_GPIO_0_27) = 0x5a000018 + 6;  //14mA +10.0dBm
    ACCESS(PADS_GPIO_0_27) = 0x5a000018 + 7; // 16mA +10.6dBm

    // Set the divider
    // cout << divisor << endl;
    // cout << divisor*pow(2.0,12) << endl;
    int div_val = (0x5a << 24) + ((int)(divisor * pow(2.0, 12)));
    // cout << hex << div_val << dec << endl;
    ACCESS(CM_GP0DIV) = div_val;

    // Turn on
    struct GPCTL setupword6 = {6 /*SRC*/, 1, 0, 0, 0, 3, 0x5a};
    struct GPCTL setupword1 = {1 /*SRC*/, 1, 0, 0, 0, 3, 0x5a};
    struct GPCTL setupword;
    if (source == PLLD)
    {
        setupword = setupword6;
    }
    else
    {
        setupword = setupword1;
    }
    ACCESS(CM_GP0CTL) = *((int *)&setupword);
}

void txoff()
{
    struct GPCTL setupword = {6 /*SRC*/, 0, 0, 0, 0, 1, 0x5a};
    ACCESS(CM_GP0CTL) = *((int *)&setupword);
}

void handSig(const int h)
{
    exit(0);
}

//
// Set up a memory regions to access GPIO
//
void setup_io(
    int &mem_fd,
    char *&gpio_mem,
    char *&gpio_map,
    volatile unsigned *&gpio) unsigned gpio_base = (bcm_host_get_peripheral_address() + 0x200000);
{
    /* open /dev/mem */
    if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
    {
        printf("can't open /dev/mem \n");
        exit(-1);
    }

    /* mmap GPIO */

    // Allocate MAP block
    if ((gpio_mem = (char *)malloc(BLOCK_SIZE + (PAGE_SIZE - 1))) == NULL)
    {
        printf("allocation error \n");
        exit(-1);
    }

    // Make sure pointer is on 4K boundary
    if ((unsigned long)gpio_mem % PAGE_SIZE)
        gpio_mem += PAGE_SIZE - ((unsigned long)gpio_mem % PAGE_SIZE);

    // Now map it
    gpio_map = (char *)mmap(
        gpio_mem,
        BLOCK_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_FIXED,
        mem_fd,
        gpio_base);

    if ((long)gpio_map < 0)
    {
        printf("mmap error %ld\n", (long int)gpio_map);
        exit(-1);
    }

    // Always use volatile pointer!
    gpio = (volatile unsigned *)gpio_map;
}

void setup_gpios(
    volatile unsigned *&gpio)
{
    int g;
    // Switch GPIO 7..11 to output mode

    /************************************************************************\
     * You are about to change the GPIO settings of your computer.          *
     * Mess this up and it will stop working!                               *
     * It might be a good idea to 'sync' before running this program        *
     * so at least you still have your code changes written to the SD-card! *
    \************************************************************************/

    // Set GPIO pins 7-11 to output
    for (g = 7; g <= 11; g++)
    {
        INP_GPIO(g); // must use INP_GPIO before we can use OUT_GPIO
                     // OUT_GPIO(g);
    }
}

void print_usage()
{
    cout << "Usage:" << endl;
    cout << "  gpioclk [source options] [frequency options]" << endl;
    cout << endl;
    cout << "Options:" << endl;
    cout << "  -s --source PLLD|XTAL" << endl;
    cout << "    Choose GIO clock source. PLLD=500MHz nominal, XTAL=19.2MHz nominal" << endl;
    cout << "    Default: PLLD" << endl;
    cout << "  -f --freq f" << endl;
    cout << "    Desired output frequency in Hz" << endl;
    cout << "  -d --divisor d" << endl;
    cout << "    Set the frequency divider value directly" << endl;
    cout << endl;
    cout << "Either --freq or --divisor (but not both) must be specified." << endl;
    cout << "Divisor is a floating point number where the integer portion is 12 bits wide" << endl;
    cout << "and the fractional portion is also 12 bits wide." << endl;
}

void parse_commandline(
    // Inputs
    const int &argc,
    char *const argv[],
    // Outputs
    source_t &source,
    bool &freq_specified,
    double &freq,
    bool &div_specified,
    double &divisor)
{
    // Default values
    source = PLLD;
    freq_specified = false;
    freq = 0;
    div_specified = false;
    divisor = 0;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"source", required_argument, 0, 's'},
        {"freq", required_argument, 0, 'f'},
        {"divisor", required_argument, 0, 'd'},
        {0, 0, 0, 0}};

    while (1)
    {
        /* getopt_long stores the option index here. */
        int option_index = 0;
        int c = getopt_long(argc, argv, "hs:f:d:",
                            long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
            char *endp;
        case 0:
            // Code should only get here if a long option was given a non-null
            // flag value.
            cout << "Check code!" << endl;
            exit(-1);
            break;
        case 'h':
            print_usage();
            exit(-1);
            break;
        case 's':
            if (!strcasecmp(optarg, "PLLD"))
            {
                source = PLLD;
            }
            else if (!strcasecmp(optarg, "XTAL"))
            {
                source = XTAL;
            }
            else
            {
                cerr << "Error: unrecognized frequency source" << endl;
                exit(-1);
            }
            break;
        case 'f':
            freq_specified = true;
            freq = strtod(optarg, &endp);
            if ((optarg == endp) || (*endp != '\0'))
            {
                cerr << "Error: could not parse frequency value" << endl;
                exit(-1);
            }
            break;
        case 'd':
            div_specified = true;
            divisor = strtod(optarg, &endp);
            if ((optarg == endp) || (*endp != '\0'))
            {
                cerr << "Error: could not parse frequency value" << endl;
                exit(-1);
            }
            break;
        case '?':
            /* getopt_long already printed an error message. */
            exit(-1);
        default:
            exit(-1);
        }
    }

    if (optind != argc)
    {
        cerr << "Error: unrecognized command line options" << endl;
        exit(-1);
    }

    // Check consistency among command line options.
    if (freq_specified && div_specified)
    {
        cerr << "Error: cannot specify both --freq and --divisor" << endl;
        exit(-1);
    }
    if ((!freq_specified) && (!div_specified))
    {
        cerr << "Error: must specify either --freq or --divisor" << endl;
        exit(-1);
    }
    if (freq_specified && (freq <= 0))
    {
        cerr << "Error: frequency must be positive" << endl;
        exit(-1);
    }
    if (div_specified && (divisor <= 0))
    {
        cerr << "Error: divisor must be positive" << endl;
        exit(-1);
    }

    // Print a summary of the parsed options
    cout << "Clock source: " << ((source == PLLD) ? "PLLD" : "XTAL") << endl;
    if (freq_specified)
    {
        cout << "Requested frequency (nominal): " << setprecision(10) << freq << " Hz" << endl;
    }
    else
    {
        cout << "Requested divisor: " << setprecision(20) << divisor << endl;
    }
}

int main(const int argc, char *const argv[])
{
    // Parse arguments
    source_t source;
    bool freq_specified;
    double freq;
    bool div_specified;
    double divisor;
    parse_commandline(
        argc,
        argv,
        source,
        freq_specified,
        freq,
        div_specified,
        divisor);

    // Nominal clock frequencies
    double f_xtal = 19200000.0;
    // PLLD clock frequency.
    // For RPi1, after NTP converges, these is a 2.5 PPM difference between
    // the PPM correction reported by NTP and the actual frequency offset of
    // the crystal. This 2.5 PPM offset is not present in the RPi2 and RPi3 (RPI4).
    // This 2.5 PPM offset is compensated for here, but only for the RPi1.
    double f_plld_clk;
    switch (ver)
    {
    case 0:
        f_plld_clk = (500000000.0 * (1 - 2.500e-6));
        break;
    case 1:
    case 2:
        f_plld_clk = (500000000.0);
        break;
    case 3:
        f_plld_clk = (750000000.0);
        break;
    default:
        fprintf(stderr, "Error: Unknown chipset (%d).", ver);
        exit(-1);
    }

    const double source_freq = (source == PLLD) ? f_plld_clk : f_xtal;
    const double divisor_max = pow(2.0, 13) - 2 + (1 - pow(2.0, -12));

    // Calculate the actual divisor
    double divisor_actual;
    if (freq_specified)
    {
        divisor = source_freq / freq;
    }
    divisor_actual = divisor;
    if (divisor_actual * pow(2.0, 12) != floor(divisor_actual * pow(2.0, 12)))
    {
        divisor_actual = floor(divisor_actual * pow(2.0, 12) + 0.5) / pow(2.0, 12);
    }
    if (divisor_actual > divisor_max)
    {
        divisor_actual = divisor_max;
    }
    if (divisor_actual < 2)
    {
        divisor_actual = 2;
    }

    // Actual frequency
    const double freq_actual = source_freq / divisor_actual;

    cout << "Actual frequency produced (nominal): " << setprecision(30) << freq_actual << " Hz" << endl;
    cout << "Actual divisor used: " << setprecision(30) << divisor_actual << endl;

    // Initial configuration
    int mem_fd;
    char *gpio_mem, *gpio_map;
    volatile unsigned *gpio = NULL;
    setup_io(mem_fd, gpio_mem, gpio_map, gpio);
    setup_gpios(gpio);
    allof7e = (unsigned *)mmap(
        NULL,
        0x002FFFFF, // len
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        mem_fd,
        BCM2708_PERI_BASE // base
    );
    if ((long int)allof7e == -1)
    {
        cerr << "Error: mmap error!" << endl;
        exit(-1);
    }

    cout << "Press CTRL-C to stop / exit" << endl;
    atexit(txoff);
    signal(SIGINT, handSig);
    signal(SIGTERM, handSig);
    signal(SIGHUP, handSig);
    signal(SIGQUIT, handSig);

    txon(source, divisor_actual);

    // Wait forever
    while (1)
    {
        usleep(1000000);
    }

    return 0;
}
