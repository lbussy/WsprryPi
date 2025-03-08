/**
 * @file main.cpp
 * @brief Test suite for the WSPRBandLookup class.
 *
 * This file contains various test functions that validate the functionality
 * of the WSPRBandLookup class, including WSPR frequency lookup, band
 * validation, frequency formatting, and error handling.
 */

#include <iostream>
#include "wspr_band_lookup.hpp"

void test_lookup(WSPRBandLookup &lookup);
void test_validate_frequency(WSPRBandLookup &lookup);
void test_freq_display_string(WSPRBandLookup &lookup);
void test_parse_frequency_string(WSPRBandLookup &lookup);
void test_error_handling(WSPRBandLookup &lookup);
void test_wspr_frequencies(WSPRBandLookup &lookup);

/** @brief Runs all test functions for WSPRBandLookup. */
int main()
{
    WSPRBandLookup lookup;

    std::cout << "\n### Running WSPRBandLookup Tests ###\n\n";

    test_lookup(lookup);
    test_validate_frequency(lookup);
    test_freq_display_string(lookup);
    test_parse_frequency_string(lookup);
    test_error_handling(lookup);
    test_wspr_frequencies(lookup);

    std::cout << "\n### All Tests Completed ###\n";
    return 0;
}

/**
 * @brief Tests WSPR frequency lookup using band names.
 *
 * Retrieves predefined WSPR transmission frequencies for standard bands
 * and prints their human-readable representation.
 *
 * @param lookup Reference to the WSPRBandLookup instance.
 */
void test_lookup(WSPRBandLookup &lookup)
{
    std::cout << "\n[TEST] WSPR Frequency Lookup:\n";
    std::string bands[] = {"40m", "160m", "10m", "6m", "2m"};

    for (const auto &band : bands)
    {
        try
        {
            double freq = std::get<double>(lookup.lookup(band));
            std::cout << band << " -> " << lookup.freq_display_string(freq) << '\n';
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << '\n';
        }
    }
}

/**
 * @brief Tests whether numeric frequencies belong to known ham bands.
 *
 * Verifies that given frequencies are correctly identified as belonging
 * to a ham band or flagged as invalid.
 *
 * @param lookup Reference to the WSPRBandLookup instance.
 */
void test_validate_frequency(WSPRBandLookup &lookup)
{
    std::cout << "\n[TEST] Ham Band Validation:\n";
    long long freqs[] = {14000000, 50000000, 28126100, 999};

    for (long long freq : freqs)
    {
        std::string band = std::get<std::string>(lookup.lookup(static_cast<int>(freq)));
        std::cout << lookup.freq_display_string(freq) << " -> " << band << '\n';
    }
}

/**
 * @brief Tests frequency display formatting.
 *
 * Converts numeric frequencies into human-readable formats using
 * appropriate GHz, MHz, kHz, or Hz units.
 *
 * @param lookup Reference to the WSPRBandLookup instance.
 */
void test_freq_display_string(WSPRBandLookup &lookup)
{
    std::cout << "\n[TEST] Frequency Display Formatting:\n";
    long long freqs[] = {14000000, 28126100, 14449050, 10000000000};

    for (long long freq : freqs)
    {
        std::cout << freq << " Hz -> " << lookup.freq_display_string(freq) << '\n';
    }
}

/**
 * @brief Tests parsing of human-readable frequency strings into Hz.
 *
 * Converts frequency strings with units (GHz, MHz, kHz, Hz) into
 * numeric values and verifies their correctness.
 *
 * @param lookup Reference to the WSPRBandLookup instance.
 */
void test_parse_frequency_string(WSPRBandLookup &lookup)
{
    std::cout << "\n[TEST] Parse Frequency Strings:\n";
    std::string inputs[] = {"7.040MHz", "10 GHz", "144.490 MHz", "475.812 kHz"};

    for (const auto &input : inputs)
    {
        try
        {
            long long freq = lookup.parse_frequency_string(input);
            std::cout << input << " -> " << lookup.freq_display_string(freq) << '\n';
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << '\n';
        }
    }
}

/**
 * @brief Tests error handling for invalid inputs.
 *
 * Ensures that invalid WSPR band names and improperly formatted frequency
 * strings are correctly rejected with appropriate error messages.
 *
 * @param lookup Reference to the WSPRBandLookup instance.
 */
void test_error_handling(WSPRBandLookup &lookup)
{
    std::cout << "\n[TEST] Error Handling for Invalid Inputs:\n";
    std::string invalid_bands[] = {"99m", "ABC123", ""};
    std::string invalid_freqs[] = {"7.040XBz", "GHz10", "MHz"};

    for (const auto &band : invalid_bands)
    {
        try
        {
            lookup.lookup(band);
            std::cerr << "Unexpected success for band: " << band << '\n';
        }
        catch (const std::exception &e)
        {
            std::cerr << "Expected failure for '" << band << "': " << e.what() << '\n';
        }
    }

    for (const auto &freq : invalid_freqs)
    {
        try
        {
            lookup.parse_frequency_string(freq);
            std::cerr << "Unexpected success for freq: " << freq << '\n';
        }
        catch (const std::exception &e)
        {
            std::cerr << "Expected failure for '" << freq << "': " << e.what() << '\n';
        }
    }
}

/**
 * @brief Verifies that all predefined WSPR frequencies are correctly loaded.
 *
 * Calls `print_wspr_frequencies()` to display the available bands and their
 * corresponding WSPR transmission frequencies.
 *
 * @param lookup Reference to the WSPRBandLookup instance.
 */
void test_wspr_frequencies(WSPRBandLookup &lookup)
{
    std::cout << "\n[TEST] Verify Loaded WSPR Frequencies:\n";
    lookup.print_wspr_frequencies();
}
