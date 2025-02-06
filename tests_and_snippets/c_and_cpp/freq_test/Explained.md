# Frequency Check Unit Test Explained

## **1. Header Files**:

```cpp
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include <stdexcept>
#include <algorithm> // For std::transform
#include <iomanip>   // For std::fixed, std::setprecision
```

These are **standard C++ libraries** used in the code:

- **`<iostream>`**: For input and output operations (like `std::cout` and `std::cin` to print to and read from the console).
- **`<sstream>`**: Allows reading strings as streams, which helps in processing inputs like space-separated frequency strings.
- **`<string>`**: Provides the `std::string` class for manipulating strings.
- **`<vector>`**: Provides the `std::vector` class for dynamic array-like data structures.
- **`<unordered_map>`**: Provides the `std::unordered_map` container, which is an efficient hash table-based map for storing key-value pairs. It's used for looking up frequency band references.
- **`<regex>`**: Provides support for regular expressions, which are used to parse input strings with specific patterns, such as detecting frequency values with units like MHz or kHz.
- **`<stdexcept>`**: Used for handling exceptions like `std::invalid_argument`.
- **`<algorithm>`**: Provides useful algorithms for manipulating collections, like `std::transform`, which can change the case of a string.
- **`<iomanip>`**: Used for controlling the formatting of output (for example, controlling the number of decimal places when printing floating-point numbers).

---

## **2. Debugging Feature**:

```cpp
// Uncomment to enable debug messages
#define WSPR_DEBUG
```

- The `#define` directive is used here to enable **debugging messages**.
- If `WSPR_DEBUG` is **defined**, debugging output will be printed, which helps to track the flow of the program and its operations. If it's commented out, debugging output is suppressed.

---

## **3. Valid Frequency Ranges**:

```cpp
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
    {241000000000, 250000000000} // UHF
};
```

- **`validRanges`** is a **vector of pairs**, where each pair represents a valid frequency range.
- The frequency ranges are for different bands (e.g., 2200m, 630m, AM radio, UHF).
- Each pair contains a **start frequency** (in Hz) and an **end frequency** (in Hz). These are used to validate whether a frequency falls within the allowed range.

---

## **4. Band Reference to Frequency Map**:

```cpp
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
```

- **`convertToFreq`** is a **map** that maps band names (e.g., "160m", "40M", "AMTest") to specific frequency values (in Hz).
- The band names are in **lowercase** to allow case-insensitive lookup (e.g., "160m" or "160M" will return the same frequency).
- This map is used to easily fetch the frequency corresponding to a given band reference.

---

## **5. Parsing Band References**:

```cpp
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
```

- **`parseBandReference`** takes an entry (band name), converts it to lowercase for case-insensitive matching, and looks it up in the `convertToFreq` map.
- If the band reference is found in the map, it assigns the corresponding frequency to the `result` and returns `true`. Otherwise, it returns `false`.

---

## **6. Parsing Frequencies (Absolute or Band Reference)**:

```cpp
double parseFrequency(const std::string &entry) {
    double parsedFreq;

    // First, check if it's a valid band reference
    if (parseBandReference(entry, parsedFreq)) {
        return parsedFreq; // Return band reference frequency immediately
    }

    // If it's not a valid band reference, try parsing as an absolute frequency
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
```

- **`parseFrequency`** tries to parse the input string as either a band reference or an absolute frequency.
- First, it checks if the input is a valid band reference. If so, it returns the frequency from the map.
- If it's not a valid band reference, it attempts to parse it as an absolute frequency.
- Finally, it checks if the frequency is within the allowed valid ranges defined earlier.

---

## **7. Main Function and Testing**:

```cpp
int main(int argc, char *argv[]) {
    if (argc > 1) {
        // Parse command-line arguments as a single list of entries
        std::string input;
        for (int i = 1; i < argc; ++i) {
            input += std::string(argv[i]) + " ";
        }

        std::cout << "Parsing

 command-line input: " << input << "\n";
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
```

- The **`main`** function handles both **command-line input** and **default tests**.
- If command-line arguments are provided, they are passed as a single string to be parsed as a list of frequencies.
- If no command-line input is provided, it runs a set of predefined tests, which include various examples of frequencies and band references.

---

## Summary:
- The code parses **frequencies** from the command line or predefined list, either as **absolute frequencies** (with scientific or standard notation) or **band references** (like "160m", "AMTest").
- It performs **validation** by checking if the parsed frequency falls within the allowed valid ranges.
- Debugging messages are printed when the `WSPR_DEBUG` preprocessor directive is enabled, helping trace the flow of parsing and validation.
