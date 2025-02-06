/**
 * @file wspr_message.cpp
 * @brief A new implementation of a wspr packet generator.
 *
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * This file is part of WsprryPi, forked from threeme3/WsprryPi (no longer
 * active).
 *
 * However, this portion of new code added to the project is distributed under
 * under the MIT License. See LICENSE for more information.
 *
 * Copyright (c) 2009-2020, Ben Hoyt, All rights reserved.
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

// Unit testing:
// g++ -Wall -Werror -fmax-errors=5 -static-libgcc -Wno-psabi -lstdc++fs -std=c++17 -DDEBUG_MAIN_MESSAGE wspr_message.cpp utils.cpp -o wspr_message -lm -lbcm_host
// Test command:
// ./wspr_message

#include "wspr_message.hpp"
#include <cstdint>
#include <iostream> // For std::cout, std::endl
#include "utils.hpp"

/**
 * @brief 162-bit synchronization vector.
 */
const unsigned char sync[] = {
    1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0,
    1, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0,
    0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1,
    0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0,
    1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1,
    1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1,
    0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1,
    1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0
};

WsprMessage::WsprMessage(char *callsign, char *location, int power) {
    to_upper(callsign);
    to_upper(location);
    generateSymbols(callsign, location, power);
}

void WsprMessage::generateSymbols(char *callsign, char *location, int power) {
    symbols = new unsigned char[MSG_SIZE];
    char call[6] = {' ', ' ', ' ', ' ', ' ', ' '};

    if (isdigit(callsign[1])) {
        for (int i = 0; i < (int)strlen(callsign); i++) {
            call[1 + i] = callsign[i];
        }
    } else if (isdigit(callsign[2])) {
        for (int i = 0; i < (int)strlen(callsign); i++) {
            call[i] = callsign[i];
        }
    }

    uint32_t N = getCharValue(call[0]) * 36 + getCharValue(call[1]);
    N = N * 10 + getCharValue(call[2]);
    N = N * 27 + getCharValue(call[3]) - 10;
    N = N * 27 + getCharValue(call[4]) - 10;
    N = N * 27 + getCharValue(call[5]) - 10;

    uint32_t M1 = (179 - 10 * (location[0] - 'A') - (location[2] - '0')) * 180 +
                  (10 * (location[1] - 'A')) + (location[3] - '0');
    uint32_t M = M1 * 128 + power + 64;

    int i;
    uint32_t reg = 0;
    unsigned char reverseAddressIndex = 0;

    for (i = 0; i < MSG_SIZE; i++) {
        symbols[i] = sync[i];
    }

    for (i = 27; i >= 0; i--) {
        reg <<= 1;
        if (N & ((uint32_t)1 << i)) reg |= 1;
        symbols[reverseAddress(reverseAddressIndex)] += 2 * calculateParity(reg & 0xf2d05351L);
        symbols[reverseAddress(reverseAddressIndex)] += 2 * calculateParity(reg & 0xe4613c47L);
    }

    for (i = 21; i >= 0; i--) {
        reg <<= 1;
        if (M & ((uint32_t)1 << i)) reg |= 1;
        symbols[reverseAddress(reverseAddressIndex)] += 2 * calculateParity(reg & 0xf2d05351L);
        symbols[reverseAddress(reverseAddressIndex)] += 2 * calculateParity(reg & 0xe4613c47L);
    }

    for (i = 30; i >= 0; i--) {
        reg <<= 1;
        symbols[reverseAddress(reverseAddressIndex)] += 2 * calculateParity(reg & 0xf2d05351L);
        symbols[reverseAddress(reverseAddressIndex)] += 2 * calculateParity(reg & 0xe4613c47L);
    }
}

int WsprMessage::getCharValue(unsigned char ch) {
    if (isdigit(ch)) return ch - '0';
    if (isalpha(ch)) return 10 + toupper(ch) - 'A';
    if (ch == ' ') return 36;
    return 0;
}

unsigned char WsprMessage::reverseAddress(unsigned char &reverseAddressIndex) {
    unsigned char result = reverseBits(reverseAddressIndex++);
    while (result > 161) {
        result = reverseBits(reverseAddressIndex++);
    }
    return result;
}

unsigned char WsprMessage::reverseBits(unsigned char b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

int WsprMessage::calculateParity(uint32_t x) {
    int even = 0;
    while (x) {
        even = 1 - even;
        x = x & (x - 1);
    }
    return even;
}

WsprMessage::~WsprMessage() {
    delete[] symbols;
}

#ifdef DEBUG_MAIN_MESSAGE
int main() {
    char callsign[] = "AA0NT";
    char location[] = "EM18";
    int power = 20;

    WsprMessage message(callsign, location, power);

    std::cout << "Callsign: " << callsign << std::endl;
    std::cout << "Location: " << location << std::endl;
    std::cout << "Power: " << power << " dBm" << std::endl;

    std::cout << "Generated WSPR symbols:" << std::endl;
    for (int i = 0; i < WsprMessage::size; ++i) {
        std::cout << static_cast<int>(message.symbols[i]);
        if (i < WsprMessage::size - 1) {
            std::cout << ",";
        }
    }
    std::cout << std::endl;

    return 0;
}
#endif
