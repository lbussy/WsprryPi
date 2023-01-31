#include "gpio.hpp"

#define PAGE_SIZE (4 * 1024)
#define BLOCK_SIZE (4 * 1024)

void *gpio_map;

// I/O access
volatile unsigned *gpio;

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio + ((g) / 10)) &= ~(7 << (((g) % 10) * 3))
#define OUT_GPIO(g) *(gpio + ((g) / 10)) |= (1 << (((g) % 10) * 3))
#define SET_GPIO_ALT(g, a) *(gpio + (((g) / 10))) |= (((a) <= 3 ? (a) + 4 : (a) == 4 ? 3 : 2) << (((g) % 10) * 3))
#define GPIO_SET *(gpio + 7)    // sets bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio + 10) // clears bits which are 1 ignores bits which are 0
#define GET_GPIO(g) (*(gpio + 13) & (1 << g)) // 0 if LOW, (1<<g) if HIGH
#define GPIO_PULL *(gpio + 37) // Pull up/pull down
#define GPIO_PULLCLK0 *(gpio + 38) // Pull up/pull down clock

void setup_io()
{
    int mem_fd;
    // Set up a memory regions to access GPIO
    unsigned gpio_base = gpioBase() + 0x200000;

    if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
    {
        printf("Fail: Unable to open /dev/mem\n");
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
        printf("Fail: mmap error %d\n", (int)gpio_map); // errno also set!
        exit(-1);
    }

    // Always use volatile pointer!
    gpio = (volatile unsigned *)gpio_map;

} // setup_io

void setupGPIO(int pin)
{
    // Set up gpio pointer for direct register access
    setup_io();

    // Set GPIO pins to output
    // Must use INP_GPIO before we can use OUT_GPIO
    INP_GPIO(pin);
    OUT_GPIO(pin);
}

void ledOn(int pin)
{
    GPIO_SET = 1 << pin;
}

void ledOff(int pin)
{
    GPIO_CLR = 1 << pin;
}

int main()
{
    printf("\nRunning on: %s.\n", version());
    printf("Testing GPIO.\n");
    setupGPIO(LED_PIN);

    for (int i = 0; i < 5; i++)
    {
        ledOn(LED_PIN);
        sleep(1);
        ledOff(LED_PIN);
        if (i < 5)
            sleep(1);
    }
    printf("Test complete.\n");

    return 0;
}
