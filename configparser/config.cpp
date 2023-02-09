#include <iostream>
#include <string>
#include "configparser.hpp"

int main()
{
    // Using callsign N9NNN, locator EM10, and TX power 33 dBm,
    // transmit a single WSPR transmission on the 20m band using
    // NTP-based frequency offset calibration.
    // sudo ./wspr N9NNN EM10 33 20m

    // Load the configuration file
    ConfigParser config = ConfigParser("config.ini");
    // Get the values of the parameters from the config file
    // std::string call_sign = config.aConfig<std::string>("Common", "Call Sign");
    // std::string grid_square = config.aConfig<std::string>("Common", "Grid Square");
    int tx_power = config.aConfig<int>("Common", "TX Power");
    std::string freq1 = config.aConfig<std::string>("Common", "Frequency");
    // Use the values for wspr

    // Call Sign = AA0NT
    // Grid Square = 
    // TX Power = 
    // Frequency 1 = 
    // std::cout << "Call sign: " << call_sign << std::endl;
    // std::cout << "Grid Square: " << grid_square << std::endl;
    std::cout << "TX Power: " << tx_power << std::endl;
    std::cout << "Frequency: " << freq1 << std::endl;
    return 0;
}