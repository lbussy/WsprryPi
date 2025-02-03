/**
 * @file wspr_message.cpp
 * @brief WSPR message generation and encoding
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
