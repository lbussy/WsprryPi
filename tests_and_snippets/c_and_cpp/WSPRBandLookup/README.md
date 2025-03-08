<!-- omit in toc -->
# WSPRBandLookup Class Documentation

<!-- omit in toc -->
## Table of Contents

- [📌 Overview](#-overview)
- [🏰️ Class Definition](#️-class-definition)
- [🛠️ Functionality \& Methods](#️-functionality--methods)
  - [🔹 `lookup(const std::variant<std::string, double, int>& input) const`](#-lookupconst-stdvariantstdstring-double-int-input-const)
  - [🔹 `freq_display_string(long long frequency) const`](#-freq_display_stringlong-long-frequency-const)
  - [🔹 `parse_frequency_string(const std::string &freq_str) const`](#-parse_frequency_stringconst-stdstring-freq_str-const)
  - [🔹 `print_wspr_frequencies() const`](#-print_wspr_frequencies-const)
- [🚀 Usage Example](#-usage-example)
  - [✅ Example Output](#-example-output)
- [🔥 Error Handling](#-error-handling)
- [📌 Summary](#-summary)
- [🎯 Next Steps](#-next-steps)

## 📌 Overview

The `WSPRBandLookup` class provides a **lookup system for WSPR frequencies** and **validates amateur radio band frequencies**. It includes:

- **WSPR frequency lookup** by band name (e.g., `"40m"` → `7040100 Hz`).
- **Validation of frequency ranges** (e.g., `14000000 Hz` → `"20M"`).
- **Formatted display of frequencies** (GHz, MHz, kHz, Hz).
- **Parsing of frequency strings** (e.g., `"7.040 MHz"` → `7040000 Hz`).
- **Error handling** for invalid inputs.

## 🏰️ Class Definition

```cpp
class WSPRBandLookup {
public:
    WSPRBandLookup();

    std::variant<double, std::string> lookup(const std::variant<std::string, double, int>& input) const;

    std::string freq_display_string(long long frequency) const;

    long long parse_frequency_string(const std::string &freq_str) const;

    void print_wspr_frequencies() const;
};
```

## 🛠️ Functionality & Methods

### 🔹 `lookup(const std::variant<std::string, double, int>& input) const`

- **Purpose**: Retrieves a WSPR frequency from a band name OR validates if a given frequency is in a ham band.
- **Inputs**:
  - `std::string`: A WSPR band name (e.g., `"40m"`).
  - `double` or `int`: A numeric frequency in Hz (e.g., `14000000` Hz).
- **Outputs**:
  - If a **band name** is given, it returns a `double` (WSPR frequency in Hz).
  - If a **numeric frequency** is given, it returns a `std::string` (Ham band name).
  - If invalid, throws an `std::invalid_argument` exception.
- **Example Usage**:

  ```cpp
  WSPRBandLookup lookup;
  double freq = std::get<double>(lookup.lookup("40m")); // 7040100 Hz
  std::string band = std::get<std::string>(lookup.lookup(14000000)); // "20M"
  ```

---

### 🔹 `freq_display_string(long long frequency) const`

- **Purpose**: Converts a numeric frequency into a **human-readable format** (GHz, MHz, kHz, Hz).
- **Inputs**:
  - `long long`: The frequency in Hz.
- **Outputs**:
  - `std::string`: A formatted string representation of the frequency.
- **Example Output**:

  ```cpp
  lookup.freq_display_string(7040100);  // "7.040100 MHz"
  lookup.freq_display_string(475812);   // "475.812 kHz"
  lookup.freq_display_string(10000000000); // "10.000000000 GHz"
  ```

---

### 🔹 `parse_frequency_string(const std::string &freq_str) const`

- **Purpose**: Converts **human-readable frequency strings** into **Hz** (e.g., `"7.040 MHz"` → `7040100 Hz`).
- **Inputs**:
  - `std::string`: A frequency string with **GHz, MHz, kHz, Hz**.
- **Outputs**:
  - `long long`: The frequency in Hz.
  - Throws `std::invalid_argument` for invalid formats.
- **Example Usage**:

  ```cpp
  long long freq = lookup.parse_frequency_string("7.040 MHz");  // 7040100 Hz
  long long freq2 = lookup.parse_frequency_string("10 GHz");    // 10000000000 Hz
  ```

- **Invalid Cases**:

  ```cpp
  lookup.parse_frequency_string("7.040XBz"); // Error: Invalid frequency format
  lookup.parse_frequency_string("GHz10");    // Error: Invalid frequency format
  ```

---

### 🔹 `print_wspr_frequencies() const`

- **Purpose**: Displays **all preloaded WSPR band frequencies** in a formatted output.
- **Outputs**:

  ``` text
  lf -> 137.500 kHz
  lf-15 -> 137.613 kHz
  mf -> 475.700 kHz
  160m -> 1.838100 MHz
  40m -> 7.040100 MHz
  10m -> 28.126100 MHz
  6m -> 50.294500 MHz
  ```

---

## 🚀 Usage Example

```cpp
#include <iostream>
#include "wspr_band_lookup.hpp"

int main() {
    WSPRBandLookup lookup;

    // Lookup a WSPR frequency
    double freq = std::get<double>(lookup.lookup("40m"));
    std::cout << "40m WSPR Frequency: " << lookup.freq_display_string(freq) << "\n";

    // Validate a frequency
    std::string band = std::get<std::string>(lookup.lookup(14000000));
    std::cout << "14000000 Hz belongs to: " << band << "\n";

    // Parse human-readable frequency string
    long long parsed_freq = lookup.parse_frequency_string("7.040 MHz");
    std::cout << "Parsed Frequency: " << parsed_freq << " Hz\n";

    // Display all WSPR frequencies
    lookup.print_wspr_frequencies();

    return 0;
}
```

### ✅ Example Output

``` text
40m WSPR Frequency: 7.040100 MHz
14000000 Hz belongs to: 20M
Parsed Frequency: 7040100 Hz
lf -> 137.500 kHz
lf-15 -> 137.613 kHz
mf -> 475.700 kHz
...
```

---

## 🔥 Error Handling

This class includes **robust error handling**:

- **Invalid WSPR band lookup**: Throws an error if an unrecognized band name is used.
- **Invalid frequency parsing**: Throws an error if a frequency string is **malformed**.
- **Invalid input types**: The `lookup()` method enforces strict type checking.

---

## 📌 Summary

| **Feature**                 | **Description** |
|-----------------------------|----------------|
| **WSPR Band Lookup**        | Retrieve the WSPR frequency for a given band. |
| **Frequency Validation**     | Check if a numeric frequency belongs to a ham band. |
| **Formatted Frequency Display** | Convert frequencies to GHz, MHz, kHz, or Hz. |
| **Parse Frequency Strings**  | Convert `"7.040 MHz"` into `7040100 Hz`. |
| **Error Handling**           | Prevents invalid lookups and parsing errors. |

---

## 🎯 Next Steps

- 👉 **Integrate into WSPR-related projects** for frequency validation.
- 👉 **Enhance with external configuration file support** for dynamic updates.
- 👉 **Optimize lookup performance** for real-time applications.

---

🚀 **This class is ready for real-world WSPR applications!** 🚀
