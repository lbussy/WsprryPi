/**
 * @file hardware_access.hpp
 * @brief This header file contains inline functions used for memory access.
 *
 * This file is part of WsprryPi, a project originally forked from
 * threeme3/WsprryPi (no longer active on GitHub).
 *
 * However, this new code added to the project is licensed under the
 * MIT License. See LICENSE.MIT.md for more information.
 *
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
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

 /*
Note on accessing memory in RPi:

There are 3 (yes three) address spaces in the Pi:
Physical addresses
  These are the actual address locations of the RAM and are equivalent
  to offsets into /dev/mem.
  The peripherals (DMA engine, PWM, etc.) are located at physical
  address 0x2000000 for RPi1 and 0x3F000000 for RPi2/3.
Virtual addresses
  These are the addresses that a program sees and can read/write to.
  Addresses 0x00000000 through 0xBFFFFFFF are the addresses available
  to a program running in user space.
  Addresses 0xC0000000 and above are available only to the kernel.
  The peripherals start at address 0xF2000000 in virtual space but
  this range is only accessible by the kernel. The kernel could directly
  access peripherals from virtual addresses. It is not clear to me my
  a user space application running as 'root' does not have access to this
  memory range.
Bus addresses
  This is a different (virtual?) address space that also maps onto
  physical memory.
  The peripherals start at address 0x7E000000 of the bus address space.
  The DRAM is also available in bus address space in 4 different locations:
  0x00000000 "L1 and L2 cached alias"
  0x40000000 "L2 cache coherent (non allocating)"
  0x80000000 "L2 cache (only)"
  0xC0000000 "Direct, uncached access"

Accessing peripherals from user space (virtual addresses):
  The technique used in this program is that mmap is used to map portions of
  /dev/mem to an arbitrary virtual address. For example, to access the
  GPIO's, the gpio range of addresses in /dev/mem (physical addresses) are
  mapped to a kernel chosen virtual address. After the mapping has been
  set up, writing to the kernel chosen virtual address will actually
  write to the GPIO addresses in physical memory.

Accessing RAM from DMA engine
  The DMA engine is programmed by accessing the peripheral registers but
  must use bus addresses to access memory. Thus, to use the DMA engine to
  move memory from one virtual address to another virtual address, one needs
  to first find the physical addresses that corresponds to the virtual
  addresses. Then, one needs to find the bus addresses that corresponds to
  those physical addresses. Finally, the DMA engine can be programmed. i.e.
  DMA engine access should use addresses starting with 0xC.

The perhipherals in the Broadcom documentation are described using their bus
addresses and structures are created and calculations performed in this
program to figure out how to access them with virtual addresses.
*/

#ifndef HARDWARE_ACCESS_HPP
#define HARDWARE_ACCESS_HPP

#include <cstdint>
#include <stdexcept>

/**
 * @brief External pointer to the virtual base address for peripherals.
 *
 * This pointer is initialized via `mmap()` at runtime to provide access
 * to the Raspberry Pi's peripheral memory space. It must be properly set
 * before calling `access_bus_addr()`, or the function will throw an error.
 */
extern volatile unsigned *peri_base_virt;

/**
 * @brief External pointer to the GPIO registers.
 *
 * This pointer is mapped at runtime using `mmap` and `/dev/mem`.
 * It must be initialized before accessing GPIO registers.
 */
extern volatile uint32_t *gpio;

/**
 * @brief Pointer to the mapped GPIO register memory.
 *
 * This variable is initialized in `hardware_access.cpp` using `mmap()`
 * and allows direct access to GPIO registers.
 *
 * @note Ensure `setupGPIO()` is called before accessing this variable.
 */
extern volatile uint32_t *gpio_map; // TODO:  Not used? I think this was for LED

/**
 * @brief Computes the virtual address for accessing a bus address.
 *
 * This function calculates the appropriate virtual address needed to access
 * a given bus address in the peripheral address space. The bus address is first
 * adjusted by subtracting `0x7e000000`, which aligns it to the base of the
 * peripheral address range. The result is then added to `peri_base_virt`,
 * the base virtual address mapped to the peripherals.
 *
 * @param bus_addr The bus address in the peripheral address space.
 * @return A reference to a volatile integer at the computed virtual address.
 *
 * @warning The function assumes `peri_base_virt` has been properly initialized
 *          via `mmap()`. Accessing an invalid bus address may cause a segmentation fault.
 */
inline volatile int& access_bus_addr(std::uintptr_t bus_addr)
{
    constexpr std::uintptr_t PERIPHERAL_BASE = 0x7e000000;

    if (!peri_base_virt) // Safety check: Ensure the base address is set
    {
        throw std::runtime_error("Peripheral base virtual address is not initialized.");
    }

    return *reinterpret_cast<volatile int*>(
        reinterpret_cast<std::uintptr_t>(peri_base_virt) + (bus_addr - PERIPHERAL_BASE));
}

/**
 * @brief Sets a specific bit in a peripheral register.
 *
 * This function sets the specified bit in the register located at the given
 * `bus_addr` by performing a bitwise OR operation.
 *
 * @param bus_addr The bus address of the register.
 * @param bit The bit position to set (0-based index).
 */
inline void set_bit(std::uintptr_t bus_addr, uint8_t bit)
{
    access_bus_addr(bus_addr) |= (1U << bit);
}

/**
 * @brief Clears a specific bit in a peripheral register.
 *
 * This function clears the specified bit in the register located at the given
 * `bus_addr` by performing a bitwise AND operation with the bit inverted.
 *
 * @param bus_addr The bus address of the register.
 * @param bit The bit position to clear (0-based index).
 */
inline void clear_bit(std::uintptr_t bus_addr, uint8_t bit)
{
    access_bus_addr(bus_addr) &= ~(1U << bit);
}

/**
 * @brief Configures a GPIO pin as an input.
 *
 * This function modifies the GPIO function select register to configure
 * the specified pin as an input. Always call this before setting a pin as an output.
 *
 * @param g The GPIO pin number (0-53 on Raspberry Pi).
 * @throws std::runtime_error If `gpio` is not initialized.
 */
inline void set_gpio_input(uint8_t g)
{
    if (!gpio)
    {
        throw std::runtime_error("GPIO base address is not initialized.");
    }

    gpio[g / 10] &= ~(7U << ((g % 10) * 3));
}

/**
 * @brief Configures a GPIO pin as an output.
 *
 * This function modifies the GPIO function select register to configure
 * the specified pin as an output. Ensure you first call `set_gpio_input(g)`
 * before setting it as an output.
 *
 * @param g The GPIO pin number (0-53 on Raspberry Pi).
 * @throws std::runtime_error If `gpio` is not initialized.
 */
inline void set_gpio_output(uint8_t g)
{
    if (!gpio)
    {
        throw std::runtime_error("GPIO base address is not initialized.");
    }

    gpio[g / 10] |= (1U << ((g % 10) * 3));
}

/**
 * @brief Configures a GPIO pin to an alternate function.
 *
 * This function modifies the GPIO function select register to configure
 * the specified pin for an alternate function mode.
 *
 * @param g The GPIO pin number (0-53 on Raspberry Pi).
 * @param a The alternate function mode (0-5).
 * @throws std::runtime_error If `gpio` is not initialized.
 * @throws std::invalid_argument If `a` is out of the valid range.
 */
inline void set_gpio_alt(uint8_t g, uint8_t a)
{
    if (!gpio)
    {
        throw std::runtime_error("GPIO base address is not initialized.");
    }

    if (a > 5)
    {
        throw std::invalid_argument("Invalid alternate function mode. Must be between 0 and 5.");
    }

    gpio[g / 10] |= (((a <= 3) ? (a + 4) : (a == 4) ? 3 : 2) << ((g % 10) * 3));
}

/**
 * @brief External pointer to the GPIO registers.
 *
 * This pointer is mapped at runtime using `mmap` and `/dev/mem`.
 * It must be initialized before accessing GPIO registers.
 */
extern volatile uint32_t *gpio;

/**
 * @brief Sets GPIO output high for the given pin.
 *
 * This function modifies the GPIO register to set a pin HIGH (1).
 * Bits that are 1 in the input will be set, while 0s are ignored.
 *
 * @param g The GPIO pin number (0-53 on Raspberry Pi).
 * @throws std::runtime_error If `gpio` is not initialized.
 */
inline void gpio_set(uint8_t g)
{
    if (!gpio)
    {
        throw std::runtime_error("GPIO base address is not initialized.");
    }
    gpio[7] = (1U << g);
}

/**
 * @brief Clears GPIO output for the given pin.
 *
 * This function modifies the GPIO register to set a pin LOW (0).
 * Bits that are 1 in the input will be cleared, while 0s are ignored.
 *
 * @param g The GPIO pin number (0-53 on Raspberry Pi).
 * @throws std::runtime_error If `gpio` is not initialized.
 */
inline void gpio_clear(uint8_t g)
{
    if (!gpio)
    {
        throw std::runtime_error("GPIO base address is not initialized.");
    }
    gpio[10] = (1U << g);
}

/**
 * @brief Reads the current state of a GPIO pin.
 *
 * This function checks if the specified GPIO pin is currently set HIGH (1) or LOW (0).
 *
 * @param g The GPIO pin number (0-53 on Raspberry Pi).
 * @return `true` if the pin is HIGH, `false` if it is LOW.
 * @throws std::runtime_error If `gpio` is not initialized.
 */
inline bool gpio_read(uint8_t g)
{
    if (!gpio)
    {
        throw std::runtime_error("GPIO base address is not initialized.");
    }
    return (gpio[13] & (1U << g)) != 0;
}

/**
 * @brief Sets the GPIO pull-up or pull-down resistor mode.
 *
 * This function writes to the GPIO pull control register to set the pull-up
 * or pull-down state for the GPIO pins.
 *
 * @param value The pull mode value to be written.
 * @throws std::runtime_error If `gpio` is not initialized.
 */
inline void gpio_set_pull(uint32_t value)
{
    if (!gpio)
    {
        throw std::runtime_error("GPIO base address is not initialized.");
    }
    gpio[37] = value;
}

/**
 * @brief Enables pull-up or pull-down settings.
 *
 * This function enables the pull-up or pull-down setting by writing to the
 * pull clock register for GPIO pins.
 *
 * @param g The GPIO pin number (0-53 on Raspberry Pi).
 * @throws std::runtime_error If `gpio` is not initialized.
 */
inline void gpio_set_pull_clock(uint8_t g)
{
    if (!gpio)
    {
        throw std::runtime_error("GPIO base address is not initialized.");
    }
    gpio[38] = (1U << g);
}

/**
 * @brief Converts a bus address to a physical address.
 *
 * This function removes the bus offset to obtain the actual physical
 * address used by the CPU and memory-mapped peripherals.
 *
 * @param bus_addr The bus address to be converted.
 * @return The corresponding physical address.
 */
inline uintptr_t bus_to_phys(uintptr_t bus_addr)
{
    return bus_addr & ~0xC0000000;
}

#endif // HARDWARE_ACCESS_HPP
