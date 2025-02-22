/**
 * @file constants.hpp
 * @brief Project constants used within the program.
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

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

// Standard library headers
#include <cstdint> // Required for uint32_t

/**
 * @brief TCP port used for singleton instance checking.
 *
 * This constant defines the port number used for checking if a singleton
 * instance of the application is already running. It allows the program to
 * prevent multiple instances from running concurrently.
 *
 * @note This feature may become redundant with `tcp_server` running.
 * @see tcp_server
 */
constexpr int SINGLETON_PORT = 1234;

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
 * @brief WSPR transmission interval for 15-minute cycles.
 *
 * This constant defines the interval, in minutes, for WSPR transmissions
 * when operating in the 15-minute mode. It is used by the scheduler to
 * determine the next transmission window.
 *
 * @see WSPR_2, wspr_interval
 */
constexpr int WSPR_15 = 15;

/**
 * @brief WSPR transmission interval for 2-minute cycles.
 *
 * This constant defines the interval, in minutes, for WSPR transmissions
 * when operating in the 2-minute mode. It is commonly used for quick 
 * transmission cycles, ensuring frequent beaconing.
 *
 * @see WSPR_15, wspr_interval
 */
constexpr int WSPR_2 = 2;

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

#endif // CONSTANTS_HPP
