#include "lcblog.hpp"
#include "ini_file.hpp"
#include "arg_parser.hpp"

LCBLog llog;
IniFile ini;

int main(const int argc, char *const argv[])
{
    llog.setLogLevel(DEBUG);

    // Parse command line arguments
    if (!parse_command_line(argc, argv))return false;

    /// Show the net configuration values after ini and command line parsing
    show_config_values();
    // Check initial load
    if (!validate_config_data()) return false;

    // TODO: The rest of the processing goes here.
}
