/**
 * @file wspr_message.hpp
 * @brief A new implementation of a wspr packet generator.
 *
 * This file is part of WsprryPi, a project originally forked from
 * threeme3/WsprryPi (no longer active on GitHub).
 *
 * However, this new code added to the project is distributed under under
 * the MIT License. See LICENSE.MIT.md for more information.
 *
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
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

#ifndef WSPR_MESSAGE_H
#define WSPR_MESSAGE_H

#include <cstdint>
#include <cstring>
#include <cctype>

/**
 * @brief Defines the size of the WSPR message in bits.
 */
#define MSG_SIZE 162

/**
 * @class WsprMessage
 * @brief Handles generation and encoding of WSPR messages.
 */
class WsprMessage {
private:
    /**
     * @brief Converts a character to its corresponding numeric value.
     *
     * @param ch The character to convert.
     * @return Numeric value of the character.
     */
    int getCharValue(unsigned char ch);

    /**
     * @brief Calculates the parity bit for a given value.
     *
     * @param ch The value to calculate parity for.
     * @return Parity bit (0 or 1).
     */
    int calculateParity(uint32_t ch);

    /**
     * @brief Reverses the bit order for address calculation.
     *
     * @param reverseAddressIndex Reference to the current address index.
     * @return Reversed bit address.
     */
    unsigned char reverseAddress(unsigned char &reverseAddressIndex);

    /**
     * @brief Reverses the bits in a byte.
     *
     * @param b The byte to reverse.
     * @return The reversed byte.
     */
    unsigned char reverseBits(unsigned char b);

    /**
     * @brief Generates WSPR symbols based on callsign, location, and power.
     *
     * @param callsign The callsign to encode.
     * @param location The Maidenhead grid location to encode.
     * @param power The power level in dBm.
     */
    void generateSymbols(char *callsign, char *location, int power);

public:
    /**
     * @brief Constructor for WsprMessage.
     *
     * @param callsign The callsign to encode.
     * @param location The Maidenhead grid location to encode.
     * @param power The power level in dBm.
     */
    WsprMessage(char *callsign, char *location, int power);

    /**
     * @brief Destructor for WsprMessage.
     * Frees allocated memory for symbols.
     */
    ~WsprMessage();

    /**
     * @brief Pointer to the generated symbols.
     */
    unsigned char *symbols;

    /**
     * @brief Size of the WSPR message in bits.
     */
    static constexpr int size = MSG_SIZE;
};

#endif // WSPR_MESSAGE_H
