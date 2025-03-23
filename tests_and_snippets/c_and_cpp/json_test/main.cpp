#include "config_handler.hpp"
#include "ini_file.hpp"
#include "json.hpp"

#include <iostream>
#include <string>

int main()
{
    // INI File
    std::string filename = "./wsprrypi.ini";
    load_json(filename);

    // Pretty-print the patched JSON with an indentation of 4 spaces.
    std::cout << "Patched config:" << std::endl;
    std::cout << jConfig.dump(4) << std::endl << std::endl;

    std::cout << "Configured INI file: " << config.ini_filename << std::endl;

    return 0;
}
