/**
 * @file wspr_band_lookup.cpp
 * @brief A C++ class designed to translate Hx to a higher order like Mhz,
 *        and back to Hz, validate frequencies, and translate terms like
 *        "20m" to a valid WSPR frequency.
 *
 * This file is part of WsprryPi, a project originally created from @threeme3
 * WsprryPi project (no longer on GitHub). However, now the original code
 * remains only as a memory and inspiration, and this project is no longer
 * a derivative work.
 *
 * This project is is licensed under the MIT License. See LICENSE.MIT.md
 * for more information.
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
     * @brief Parses an input string as a frequency, with optional validation.
     *
     * This function first checks if the input is a known WSPR band name (e.g., "20m").
     * If found, it returns the associated WSPR frequency in Hz.
     *
     * If not a known band, the function attempts to parse the input as a frequency
     * string with a unit (e.g., "7.040100 MHz"). If parsing succeeds, the resulting
     * frequency is returned in Hz.
     *
     * If the input is a plain numeric value (e.g., "7040100"), it can either be
     * validated against known ham bands or returned as-is, depending on the `validate` flag.
     *
     * @param input The input string, which can be:
     *              - A WSPR band name (e.g., "20m")
     *              - A frequency string with a unit (e.g., "7.040100 MHz")
     *              - A raw numeric value (e.g., "7040100")
     * @param validate If `true`, checks raw numeric values against known ham bands
     *                 and rejects unrecognized frequencies.
     *                 If `false`, returns the frequency as-is without validation.
     *
     * @return A `double` representing the frequency in Hz.
     *
     * @throws std::invalid_argument If the input is invalid, or if validation is enabled
     *         and the frequency does not match any known ham band.
     */
    double parse_string_to_frequency(std::string_view input, bool validate = true) const;

    /**
     * @brief Prints all predefined WSPR frequencies to standard output.
     */
    void print_wspr_frequencies() const;
};

#endif // WSPR_BAND_LOOKUP_HPP
