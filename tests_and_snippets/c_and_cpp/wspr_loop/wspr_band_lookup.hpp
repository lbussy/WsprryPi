/**
 * @file wspr_band_lookup.cpp
 * @brief A C++ class designed to translate Hx to a higher orer like Mhz,
 *        and back to Hz, validate frequencies, and translate terms like
 *        "20m" to a valid WSPR frequency.
 *
 * This file is part of WsprryPi, a project originally forked from threeme3
 * / WsprryPi (no longer active on GitHub).
 *
 * However, this new code added to the project is licensed under the
 * MIT License. See LICENSE.MIT.md for more information.
 *
 * Copyright (C) 2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef WSPR_BAND_LOOKUP_HPP
#define WSPR_BAND_LOOKUP_HPP

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <string>
#include <variant>
#include <sstream>
#include <iomanip>

/**
 * @class WSPRBandLookup
 * @brief Provides methods for WSPR frequency lookup, validation, and formatting.
 */
class WSPRBandLookup
{
private:
    /**
     * @brief Stores predefined WSPR frequencies mapped by their band name.
     */
    std::unordered_map<std::string, double> wsprFrequencies;

    /**
     * @brief Defines a frequency range tuple (start, end, band name).
     */
    using FrequencyRange = std::tuple<long long, long long, std::string>;

    /**
     * @struct TupleHash
     * @brief Custom hash function for tuple hashing in an unordered_set.
     */
    struct TupleHash
    {
        template <typename T1, typename T2, typename T3>
        std::size_t operator()(const std::tuple<T1, T2, T3> &t) const
        {
            return std::hash<T1>()(std::get<0>(t)) ^
                   (std::hash<T2>()(std::get<1>(t)) << 1);
        }
    };

    /**
     * @brief Stores valid ham radio frequency ranges and their band names.
     */
    const std::unordered_set<FrequencyRange, TupleHash> validHamFrequencies;

protected:
    /**
     * @brief Normalizes a given key (band name) to lowercase for case-
     *        insensitive lookups.
     * @param key The input string to normalize.
     * @return A lowercase version of the input string.
     */
    std::string normalize_key(const std::string &key) const;

    /**
     * @brief Validates if a given frequency belongs to a known ham band.
     * @param frequency The frequency in Hz.
     * @return The name of the corresponding ham band, or "Invalid Frequency"
     *         if not found.
     */
    std::string validate_frequency(long long frequency) const;

public:
    /**
     * @brief Constructs the WSPRBandLookup object and initializes the
     *        frequency tables.
     */
    WSPRBandLookup();

    /**
     * @brief Looks up either a WSPR frequency by band name or validates a
     *        given numeric frequency.
     * @param input The band name (std::string) or frequency (double/int)
     *              in Hz.
     * @return If given a band name, returns the WSPR frequency (double).
     *         If given a frequency, returns the corresponding ham band
     *         (std::string).
     */
    std::variant<double, std::string> lookup(const std::variant<std::string, double, int> &input) const;

    /**
     * @brief Converts a numeric frequency into a formatted string representation.
     * @param frequency The frequency in Hz.
     * @return A human-readable string representation of the frequency (GHz, MHz,
     *         kHz, Hz).
     */
    std::string freq_display_string(long long frequency) const;

    /**
     * @brief Parses a frequency string (e.g., "7.040 MHz") and converts it to Hz.
     * @param freq_str The frequency string with units (GHz, MHz, kHz, Hz).
     * @return The frequency value in Hz.
     * @throws std::invalid_argument if the input format is incorrect.
     */
    long long parse_frequency_string(const std::string &freq_str) const;

    /**
     * @brief Prints all predefined WSPR frequencies to standard output.
     */
    void print_wspr_frequencies() const;
};

#endif // WSPR_BAND_LOOKUP_HPP
