#include "main.hpp"

#include "ppm_ntp.hpp"
#include "scheduling.hpp"
#include "arg_parser.hpp"
#include "constants.hpp"
#include "signal_handler.hpp"

std::string ini_file = "./wsprrypi.ini";

int main()
{
    llog.setLogLevel(INFO);
    llog.enableTimestamps(true);
    llog.logS(INFO, "Starting handlers.");
    ini.set_filename(ini_file);           // Load the INI file
    iniMonitor.filemon(ini_file.c_str()); // Monitor the INI file

    // Handle signals and teminal supression of ctrl codes
    register_signal_handlers();

    enable_performance_mode();

    if (!ensure_ntp_stable())
        std::exit(EXIT_FAILURE); // Ensure stable NTP before anything runs

    llog.logS(DEBUG, "NTP verified at startup.");

    update_ppm();

    wspr_interval = WSPR_2;
    wspr_loop();

    return 0;
}
