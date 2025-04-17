#ifndef _WSPR_H
#define _WSPR_H

#include "config_handler.hpp"
#include "wspr_message.hpp"

#include <array>
#include <atomic>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

/**
 * @brief Page information for the constant memory page.
 *
 * This global variable holds the bus and virtual addresses of the constant memory page,
 * which is used to store fixed data required for DMA operations, such as the tuning words
 * for frequency generation.
 */
extern struct PageInfo constPage;

/**
 * @brief Page information for the DMA instruction page.
 *
 * This global variable holds the bus and virtual addresses of the DMA instruction page,
 * where DMA control blocks (CBs) are stored. This page is used during the setup and
 * operation of DMA transfers.
 */
extern struct PageInfo instrPage;

/**
 * @brief Array of page information structures for DMA control blocks.
 *
 * This global array contains the bus and virtual addresses for each page used in the DMA
 * instruction chain. It holds 1024 entries, corresponding to the 1024 DMA control blocks used
 * for managing data transfers.
 */
extern struct PageInfo instrs[1024];

/**
 * @brief Global instance of transmission parameters for WSPR.
 *
 * This global variable holds the current settings used for a WSPR transmission,
 * including the WSPR message, transmission frequency, symbol time, tone spacing,
 * and the DMA frequency lookup table.
 */
extern struct WsprTransmissionParams transParams;

/**
 * @brief DMA configuration and saved state for transmission setup/cleanup.
 *
 * @details Stores both the nominal and (PPM‑corrected) PLLD clock frequencies,
 *          mailbox memory flags, the virtual base address for peripheral access,
 *          and the original register values that must be restored when
 *          tearing down DMA/PWM configuration.
 */
struct DMAConfig
{
    double plld_nominal_freq;      ///< PLLD clock frequency in Hz before any PPM correction.
    double plld_clock_frequency;   ///< PLLD clock frequency in Hz after PPM correction.
    int mem_flag;                  ///< Mailbox memory allocation flags for DMA.
    void *peripheral_base_virtual; ///< Virtual base pointer for /dev/mem mapping of peripherals.
    uint32_t orig_gp0ctl;          ///< Saved GP0CTL register (clock control).
    uint32_t orig_gp0div;          ///< Saved GP0DIV register (clock divider).
    uint32_t orig_pwm_ctl;         ///< Saved PWM control register.
    uint32_t orig_pwm_sta;         ///< Saved PWM status register.
    uint32_t orig_pwm_rng1;        ///< Saved PWM range register 1.
    uint32_t orig_pwm_rng2;        ///< Saved PWM range register 2.
    uint32_t orig_pwm_fifocfg;     ///< Saved PWM FIFO configuration register.

    /**
     * @brief Construct a new DMAConfig with default (nominal) settings.
     *
     * @details Initializes:
     *   - `plld_nominal_freq` to 500 MHz with the built‑in 2.5 ppm correction.
     *   - `plld_clock_frequency` equal to `plld_nominal_freq`.
     *   - `mem_flag` to the default mailbox flag.
     *   - All pointers and saved‑register fields to zero or nullptr.
     */
    DMAConfig()
        : plld_nominal_freq(500000000.0 * (1 - 2.500e-6)),
          plld_clock_frequency(plld_nominal_freq),
          mem_flag(0x0c),
          peripheral_base_virtual(nullptr),
          orig_gp0ctl(0),
          orig_gp0div(0),
          orig_pwm_ctl(0),
          orig_pwm_sta(0),
          orig_pwm_rng1(0),
          orig_pwm_rng2(0),
          orig_pwm_fifocfg(0)
    {
    }
};

/**
 * @brief Global configuration object.
 *
 * This DMAConfig instance holds the transmission functionality global objects.
 */
extern struct DMAConfig dmaConfig;

/**
 * @brief Structure containing parameters for a WSPR transmission.
 *
 * This structure encapsulates all the necessary parameters required to configure
 * and execute a WSPR transmission, including the message, transmission frequency,
 * symbol time, tone spacing, and the DMA frequency lookup table.
 */
struct WsprTransmissionParams
{
    static constexpr std::size_t symbol_count = MSG_SIZE;
    std::array<uint8_t, symbol_count> symbols;

    double frequency;                   ///< Transmission frequency in Hz.
    bool is_tone;                       ///< Is test tone
    int power;                          ///< GPIO power level 0-7
    WsprMode wspr_mode;                 ///< WSPR mode for the frequency.
    double symtime;                     ///< Duration of each symbol in seconds.
    double tone_spacing;                ///< Frequency spacing between adjacent tones in Hz.
    std::vector<double> dma_table_freq; ///< DMA frequency lookup table.
    bool use_offset;                    ///< Use random offset on transmissions

    /**
     * @brief Default constructor for WsprTransmissionParams.
     *
     * Initializes the transmission parameters with default values.
     */
    WsprTransmissionParams()
        : symbols{},
          frequency(0.0),
          is_tone(false),
          power(0),
          wspr_mode(WsprMode::WSPR2),
          symtime(0.0),
          tone_spacing(0.0),
          dma_table_freq(1024, 0.0),
          use_offset(false)
    {
    }

    /**
     * @brief Clears all stored transmission parameters and symbols.
     */
    void clear()
    {
        symbols.fill(0);
        frequency = 0.0;
        is_tone = false;
        power = 0;
        wspr_mode = WsprMode::WSPR2,
        symtime = 0.0;
        tone_spacing = 0.0;
        use_offset = false;
        dma_table_freq.clear();
    }

    /**
     * @brief Prints all transmission parameters and WSPR symbols to the console.
     */
    void print() const
    {
        std::cout << std::fixed << std::setprecision(6);
        // std::setprecision(6) << std::fixed << transParams.frequency / 1.0e6 << " MHz."
        std::cout << "WSPR Frequency:     " << std::setprecision(6) << std::fixed << frequency / 1.0e6 << " MHz" << std::endl;
        std::cout << "GPIO Power:         " << power << " (0-7)" << std::endl;
        std::cout << "WSPR Mode           " << (wspr_mode == WsprMode::WSPR2 ? "WSPR-2" : "WSPR-15") << std::endl;
        std::cout << "Test Tone           " << (is_tone ? "True" : "False") << std::endl;
        std::cout << "WSPR Symbol Time:   " << symtime << " secs." << std::endl;
        std::cout << "WSPR Tone Spacing:  " << tone_spacing << " Hz" << std::endl;
        std::cout << "DMA Table Size:     " << dma_table_freq.size() << std::endl;

        // Iterate through the symbols and print them.
        std::cout << "WSPR Symbols:" << std::endl;
        int symbols_size = static_cast<int>(symbols.size());
        for (int i = 0; i < symbols_size; ++i)
        {
            // Print each symbol as an integer.
            std::cout << static_cast<int>(symbols[i]);
            if (i < symbols_size - 1)
                std::cout << ", ";

            // Optionally, insert a newline every 16 symbols for readability.
            if ((i + 1) % 18 == 0 && i < symbols_size - 1)
                std::cout << std::endl;
        }
        std::cout << std::endl;
    }
};

/**
 * @brief Global instance of transmission parameters for WSPR.
 *
 * This global variable holds the current settings used for a WSPR transmission,
 * including the WSPR message, transmission frequency, symbol time, tone spacing,
 * and the DMA frequency lookup table.
 */
extern struct WsprTransmissionParams transParams;

void dma_cleanup();
void setup_dma();
void setup_transmission(
    double frequency,
    int power = 7,
    double ppm = 0.0,
    const std::string &callsign = "",
    const std::string &grid_square = "",
    int power_dbm = 0,
    bool use_offset = false);
void transmit();

#endif // _WSPR_H
