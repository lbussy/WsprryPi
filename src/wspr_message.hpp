// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

/**
 * @file wspr_message.hpp
 * @brief Header file for the WsprMessage class used for WSPR message generation and encoding.
 *
 * Updated and maintained by Lee C. Bussy.
 *
 * Originally forked from threeme3/WsprryPi (no longer active), this project
 * has been significantly updated, improved, and documented for ease of use.
 * Inspired by a conversation with Bruce Raymond of TAPR, this fork operates
 * as an independent project.
 *
 * Contributors:
 * - threeme3 (Original Author)
 * - Bruce Raymond (Inspiration and Guidance)
 * - Lee Bussy (@LBussy)
 *
 * Copyright (C) 2023-2024 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-55ad7f3 [fix_57].
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
