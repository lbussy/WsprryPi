/**
 * @file wspr_message.hpp
 * @brief Header file for the WsprMessage class used for WSPR message generation and encoding.
 *
 * This file is part of WsprryPi, forked from threeme3/WsprryPi (no longer
 * active).
 *
 * Copyright (C) @threeme3 (unknown dates)
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * WsprryPi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
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
