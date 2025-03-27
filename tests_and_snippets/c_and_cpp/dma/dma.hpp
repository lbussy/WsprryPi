#ifndef _WSPR_H
#define _WSPR_H

#include "ppm_manager.hpp"

// Exists in config_handler
enum class ModeType
{
    WSPR, ///< WSPR transmission mode
    TONE  ///< Test tone generation mode
};

// Exists in config_handler
struct ArgParserConfig
{
    // Control
    bool transmit; ///< Transmission mode enabled.

    // Common
    std::string callsign;    ///< WSPR callsign.
    std::string grid_square; ///< 4- or 6-character Maidenhead locator.
    int power_dbm;           ///< Transmit power in dBm.
    std::string frequencies; ///< Comma-separated frequency list.
    int tx_pin;              ///< GPIO pin number for RF transmit control.

    // Extended
    double ppm;      ///< PPM frequency calibration.
    bool use_ntp;    ///< Apply NTP-based frequency correction.
    bool use_offset; ///< Enable random frequency offset.
    int power_level; ///< Power level for RF hardware (0–7).
    bool use_led;    ///< Enable TX LED indicator.
    int led_pin;     ///< GPIO pin for LED indicator.

    // Server
    int web_port;      ///< Web server port number.
    int socket_port;   ///< Socket server port number.
    bool use_shutdown; ///< Enable GPIO-based shutdown feature.
    int shutdown_pin;  ///< GPIO pin used to signal shutdown.

    // Command line only
    bool date_time_log; ///< Prefix logs with timestamp.
    bool loop_tx;       ///< Repeat transmission cycle.
    int tx_iterations;  ///< Number of transmission iterations (0 = infinite).
    double test_tone;   ///< Enable continuous tone mode (in Hz).

    // Runtime variables
    ModeType mode;                       ///< Current operating mode.
    bool use_ini;                        ///< Load configuration from INI file.
    std::string ini_filename;            ///< INI file name and path.
    std::vector<double> center_freq_set; ///< Parsed list of center frequencies in Hz.
    double f_plld_clk;                   ///< Clock speed (defaults to 500 MHz).
    int mem_flag;                        ///< Reserved for future memory management flags.

    /**
     * @brief Default constructor initializing all configuration parameters.
     */
    ArgParserConfig()
        : transmit(false),
          callsign(""),
          grid_square(""),
          power_dbm(0),
          frequencies(""),
          tx_pin(-1),
          ppm(0.0),
          use_ntp(false),
          use_offset(false),
          power_level(7),
          use_led(false),
          led_pin(-1),
          web_port(-1),
          socket_port(-1),
          use_shutdown(false),
          shutdown_pin(-1),
          date_time_log(false),
          loop_tx(false),
          tx_iterations(0),
          test_tone(0.0),
          mode(ModeType::WSPR),
          use_ini(false),
          ini_filename(""),
          center_freq_set({}),
          f_plld_clk(0.0),
          mem_flag(0)
    {
    }
};

extern ArgParserConfig config;

extern PPMManager ppmManager;


#ifdef __cplusplus
extern "C"
{
#include "mailbox.h"
}
#endif /* __cplusplus */

void dma_cleanup();
void setup_dma();
void tx_tone();
void tx_wspr();

#endif // _WSPR_H
