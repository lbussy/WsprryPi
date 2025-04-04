#ifndef _WSPR_H
#define _WSPR_H

#include <string>
#include <vector>

typedef enum
{
    WSPR,
    TONE
} mode_type;

struct wConfig
{
    // Global configuration items from command line and ini file
    bool useini = false;
    std::string inifile = "";                   // Default to empty, meaning no INI file specified
    bool xmit_enabled = true;                   // Transmission disabled by default
    bool repeat = false;                        // No repeat transmission by default
    std::string callsign = "AA0NT";             // Default to empty, requiring user input
    std::string locator = "EM18";               // Default to empty, requiring user input
    int tx_power = 20;                          // Default to 37 dBm (5W), a common WSPR power level
    std::string frequency_string = "7040100.0"; // Default to empty
    std::vector<double> center_freq_set = {};   // Empty vector, frequencies to be defined
    double ppm = 13.114;                        // Default to zero, meaning no frequency correction applied
    bool self_cal = true;                       // Self-calibration enabled by default
    bool random_offset = true;                  // No random offset by default
    double test_tone = 7040100.0;               // Default to NAN, meaning no test tone
    bool no_delay = false;                      // Delay enabled by default
    mode_type mode = WSPR;                      // Default mode is WSPR
    int terminate = -1;                         // -1 to indicate no termination signal
    bool useled = false;                        // No LED signaling by default
    bool daemon_mode = false;                   // Not running as a daemon by default
    double f_plld_clk = 125e6;                  // Default PLLD clock frequency: 125 MHz
    int mem_flag = 0;                           // Default memory flag set to 0
};

extern struct wConfig config;

void dma_cleanup();
void setup_dma();
void tx_tone();
void tx_wspr();

#endif // _WSPR_H
