#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <stdexcept>
#include <algorithm> // For std::transform
#include <iomanip>   // For std::fixed, std::setprecision

// Uncomment to enable debug messages
#define WSPR_DEBUG

// Define valid frequency ranges (in Hz) using decimal format
std::vector<std::pair<double, double>> validRanges = {
    {135700, 137800},         // 2200m
    {472000, 479000},         // 630m
    {530000, 1700000},        // AM radio broadcast range
    {1800000, 2000000},       // 160m
    {3000000, 30000000},      // 80m
    {5332000, 5405000},       // 60m
    {7000000, 7300000},       // 40m
    {10100000, 10150000},     // 30m
    {14000000, 14350000},     // 20m
    {18068000, 18110000},     // 17m
    {21025000, 21200000},     // 15m
    {24890000, 24930000},     // 12m
    {28000000, 29700000},     // 10m
    {50000000, 54000000},     // 6m
    {144000000, 148000000},   // 2m
    {222000000, 225000000},   // 1.25m
    {300000000, 2310000000},  // UHF
    {420000000, 450000000},   // 70cm
    {902000000, 928000000},   // 33cm
    {1270000000, 1300000000}, // 23cm
    {2390000000, 2450000000}, // UHF
    {3300000000, 3500000000}, // UHF
    {5650000000, 5925000000}, // UHF
    {10000000000, 10500000000}, // UHF
    {24000000000, 24250000000}, // UHF
    {47000000000, 47200000000}, // UHF
    {76000000000, 81000000000}, // UHF
    {122250000000, 123000000000}, // UHF
    {134000000000, 141000000000}, // UHF
    {241000000000, 250000000000}, // UHF
    {300000000000, 1000000000000} // UHF
};

// Arbitrary band references with representative frequencies (in Hz)
std::unordered_map<std::string, double> convertToFreq() {
    return {
        {"lf", 137500},                // LF
        {"lf-15", 137612.5},           // LF-15
        {"mf", 475700},                // MF
        {"mf-15", 475812.5},           // MF-15
        {"160m", 1838100},             // 160m
        {"160m-15", 1838212.5},        // 160m-15
        {"80m", 3570100},              // 80m
        {"60m", 5288700},              // 60m
        {"40m", 7040100},              // 40m
        {"30m", 10140200},             // 30m
        {"20m", 14097100},             // 20m
        {"17m", 18106100},             // 17m
        {"15m", 21096100},             // 15m
        {"12m", 24926100},             // 12m
        {"10m", 28126100},             // 10m
        {"6m", 50294500},              // 6m
        {"4m", 70092500},              // 4m
        {"2m", 14449050},              // 2m
        {"amtest", 780000}             // AM Test Tone at 780 kHz
    };
}

// Normalize input to lowercase for case-insensitive matching
std::string toLowerCase(const std::string &input) {
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    return lower;
}

// Check if a frequency falls within valid ranges
bool isFrequencyValid(double freq) {
    // Use range-based for loop without structured bindings for C++11
    for (const auto& range : validRanges) {
        if (freq >= range.first && freq <= range.second) {
            return true;
        }
    }
    return false;
}

// Parse an absolute frequency (decimal or scientific notation)
double parseAbsoluteFrequency(const std::string &entry) {
#ifdef WSPR_DEBUG
    std::cout << "Attempting to parse absolute frequency: " << entry << std::endl;
#endif

    // Regex to detect frequency with units (e.g., kHz, MHz, GHz)
    std::regex unit_regex(R"(([\d.]+)\s*(hz|khz|mhz|ghz)?)", std::regex_constants::icase);
    std::smatch match;

    if (std::regex_match(entry, match, unit_regex)) {
        double value = std::stod(match[1].str());
        std::string unit = toLowerCase(match[2].str());

        // Convert based on unit
        if (unit == "khz") {
            value *= 1e3;
        } else if (unit == "mhz") {
            value *= 1e6;
        } else if (unit == "ghz") {
            value *= 1e9;
        }

        return value;
    }

    // Handle raw decimal or scientific notation input
    double freq;
    try {
        freq = std::stod(entry);  // Try to parse as a regular float (decimal or scientific notation)
#ifdef WSPR_DEBUG
        std::cout << "Parsed frequency: " << freq << std::endl;
#endif
    } catch (...) {
        throw std::invalid_argument("Invalid frequency format: " + entry);
    }

    return freq;
}

// Try parsing a band reference and return its frequency, or return false if invalid
bool parseBandReference(const std::string &entry, double &result) {
    const auto freq_map = convertToFreq();
    std::string normalizedEntry = toLowerCase(entry);
    auto it = freq_map.find(normalizedEntry);
    if (it != freq_map.end()) {
        result = it->second;
        return true;  // Found valid band reference
    }
    return false; // Not a valid band reference
}

// Parse a frequency (absolute or band reference)
double parseFrequency(const std::string &entry) {
    double parsedFreq;

    // First, check if it's a valid band reference
    if (parseBandReference(entry, parsedFreq)) {
#ifdef WSPR_DEBUG
        std::cout << "Band reference found: " << entry << " with frequency: " << parsedFreq << std::endl;
#endif
        return parsedFreq; // Return band reference frequency immediately
    }

    // If it’s not a valid band reference, attempt to parse as an absolute frequency
    try {
        parsedFreq = parseAbsoluteFrequency(entry);
    } catch (const std::exception &e) {
        throw std::invalid_argument("Invalid frequency or band reference: " + entry);
    }

    // Check if the frequency is within the valid range
    if (!isFrequencyValid(parsedFreq)) {
        throw std::invalid_argument("Frequency out of valid range: " + entry);
    }

    return parsedFreq;
}

// Parse a list of frequencies
std::vector<double> parseFrequencyList(const std::string &frequency_string) {
    std::vector<double> frequencies;
    std::istringstream stream(frequency_string);
    std::string entry;
    while (stream >> entry) {
        try {
            frequencies.push_back(parseFrequency(entry));
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
    return frequencies;
}

// Testing function
void runTests() {
    std::vector<std::string> testCases = {
        "14097100",           // Valid absolute frequency
        "AMTest",             // Valid test tone
        "160m",               // Valid band reference
        "40M",                // Valid uppercase band reference
        "28.5e6",             // Valid scientific notation
        "1.4MHz",             // Valid frequency with unit
        "7000e6",             // Invalid scientific notation (out of range)
        "invalidBand"         // Invalid band reference
    };

    for (const auto &test : testCases) {
        std::cout << "\nTesting input: " << test << std::endl;
        try {
            double result = parseFrequency(test);
            std::cout << "Parsed frequency: " << std::fixed << std::setprecision(0) << result << " Hz" << std::endl;
        } catch (const std::exception &e) {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        // Parse command-line arguments as a single list of entries
        std::string input;
        for (int i = 1; i < argc; ++i) {
            input += std::string(argv[i]) + " ";
        }

        std::cout << "Parsing command-line input: " << input << "\n";
        auto frequencies = parseFrequencyList(input);
        std::cout << "Parsed frequencies:\n";
        for (double freq : frequencies) {
            std::cout << std::fixed << std::setprecision(0) << freq << " Hz\n";
        }
    } else {
        // Run default tests
        std::cout << "No command-line input provided. Running predefined tests...\n";
        runTests();
    }

    return 0;
}
