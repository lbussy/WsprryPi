// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

/*
 * WsprryPi
 * Updated and maintained by Lee C. Bussy
 *
 * Originally forked from threeme3/WsprryPi (no longer active), this
 * project has been significantly updated, improved, and documented for
 * ease of use.
 *
 * Inspired by a conversation with Bruce Raymond of TAPR, this fork has
 * diverged substantially from its origins and operates as an independent
 * project.
 *
 * Contributors:
 *   - threeme3 (Original Author)
 *   - Bruce Raymond (Inspiration and Guidance)
 *   - Lee Bussy, aa0nt@arrl.net
 *
 * Copyright (C) 2023-2024 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-55ad7f3 [fix_57].
*/

#include <cstring>  // For strncpy, strchr, strlen
#include <cctype>   // For toupper, isdigit
#include <iostream> // For std::cout

#include "wspr_encoder.hpp" // Declaration of wspr function
#include "utils.hpp"        // Declaration of to_upper function

// Test compile:
// g++ -Wall -Werror -std=c++11 -DDEBUG_MAIN wspr_encoder.cpp utils.cpp -o wspr_encoder

void wspr(
    const char *call,
    const char *l_pre,
    const char *dbm,
    unsigned char *symbols)
{
    // Encode call, grid_square, and dBm into WSPR codeblock.

    // pack prefix in nadd, call in n1, grid, dbm in n2
    char *c, buf[16];
    strncpy(buf, call, 16);
    c = buf;
    to_upper(c);
    unsigned long ng, nadd = 0;

    if (strchr(c, '/'))
    { // prefix-suffix
        nadd = 2;
        int i = strchr(c, '/') - c; // stroke position
        int n = strlen(c) - i - 1;  // suffix len, prefix-call len
        c[i] = '\0';
        if (n == 1)
            ng = 60000 - 32768 + (c[i + 1] >= '0' && c[i + 1] <= '9' ? c[i + 1] - '0' : c[i + 1] == ' ' ? 38
                                                                                                        : c[i + 1] - 'A' + 10); // suffix /A to /Z, /0 to /9
        if (n == 2)
            ng = 60000 + 26 + 10 * (c[i + 1] - '0') + (c[i + 2] - '0'); // suffix /10 to /99
        if (n > 2)
        { // prefix EA8/, right align
            ng = (i < 3 ? 36 : c[i - 3] >= '0' && c[i - 3] <= '9' ? c[i - 3] - '0'
                                                                  : c[i - 3] - 'A' + 10);
            ng = 37 * ng + (i < 2 ? 36 : c[i - 2] >= '0' && c[i - 2] <= '9' ? c[i - 2] - '0'
                                                                            : c[i - 2] - 'A' + 10);
            ng = 37 * ng + (i < 1 ? 36 : c[i - 1] >= '0' && c[i - 1] <= '9' ? c[i - 1] - '0'
                                                                            : c[i - 1] - 'A' + 10);
            if (ng < 32768)
                nadd = 1;
            else
                ng = ng - 32768;
            c = c + i + 1;
        }
    }

    int i = (isdigit(c[2]) ? 2 : isdigit(c[1]) ? 1
                                               : 0); // last prefix digit of de-suffixed/de-prefixed callsign
    int n = strlen(c) - i - 1;                       // 2nd part of call len
    unsigned long n1;
    n1 = (i < 2 ? 36 : c[i - 2] >= '0' && c[i - 2] <= '9' ? c[i - 2] - '0'
                                                          : c[i - 2] - 'A' + 10);
    n1 = 36 * n1 + (i < 1 ? 36 : c[i - 1] >= '0' && c[i - 1] <= '9' ? c[i - 1] - '0'
                                                                    : c[i - 1] - 'A' + 10);
    n1 = 10 * n1 + c[i] - '0';
    n1 = 27 * n1 + (n < 1 ? 26 : c[i + 1] - 'A');
    n1 = 27 * n1 + (n < 2 ? 26 : c[i + 2] - 'A');
    n1 = 27 * n1 + (n < 3 ? 26 : c[i + 3] - 'A');

    // if(rand() % 2) nadd=0;
    if (!nadd)
    {
        // Copy grid_square locally since it is declared const and we cannot modify
        // its contents in-place.
        char l[4];
        strncpy(l, l_pre, 4);
        to_upper(l); // grid square Maidenhead grid_square (uppercase)
        ng = 180 * (179 - 10 * (l[0] - 'A') - (l[2] - '0')) + 10 * (l[1] - 'A') + (l[3] - '0');
    }
    int p = atoi(dbm); // EIRP in dBm={0,3,7,10,13,17,20,23,27,30,33,37,40,43,47,50,53,57,60}
    int corr[] = {0, -1, 1, 0, -1, 2, 1, 0, -1, 1};
    p = p > 60 ? 60 : p < 0 ? 0
                            : p + corr[p % 10];
    unsigned long n2 = (ng << 7) | (p + 64 + nadd);

    // pack n1,n2,zero-tail into 50 bits
    char packed[11] = {
        static_cast<char>(n1 >> 20),
        static_cast<char>(n1 >> 12),
        static_cast<char>(n1 >> 4),
        static_cast<char>(((n1 & 0x0f) << 4) | ((n2 >> 18) & 0x0f)),
        static_cast<char>(n2 >> 10),
        static_cast<char>(n2 >> 2),
        static_cast<char>((n2 & 0x03) << 6),
        0,
        0,
        0,
        0};

    // convolutional encoding K=32, r=1/2, Layland-Lushbaugh polynomials
    int k = 0;
    int j, s;
    int nstate = 0;
    unsigned char symbol[176];
    for (j = 0; j != sizeof(packed); j++)
    {
        for (i = 7; i >= 0; i--)
        {
            unsigned long poly[2] = {0xf2d05351L, 0xe4613c47L};
            nstate = (nstate << 1) | ((packed[j] >> i) & 1);
            for (s = 0; s != 2; s++)
            { // convolve
                unsigned long n = nstate & poly[s];
                int even = 0; // even := parity(n)
                while (n)
                {
                    even = 1 - even;
                    n = n & (n - 1);
                }
                symbol[k] = even;
                k++;
            }
        }
    }

    // interleave symbols
    const unsigned char npr3[162] = {
        1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0,
        0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0,
        0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0,
        0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1,
        0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0,
        0, 0};
    for (i = 0; i != 162; i++)
    {
        // j0 := bit reversed_values_smaller_than_161[i]
        unsigned char j0;
        p = -1;
        for (k = 0; p != i; k++)
        {
            for (j = 0; j != 8; j++) // j0:=bit_reverse(k)
                j0 = ((k >> j) & 1) | (j0 << 1);
            if (j0 < 162)
                p++;
        }
        symbols[j0] = npr3[j0] | symbol[i] << 1; // interleave and add sync std::vector
    }
}

// Debug-only main function
#ifdef DEBUG_MAIN_ENCODER
int main() {
    const char* call = "K1ABC";
    const char* grid = "FN42";
    const char* power = "10";
    unsigned char symbols[162] = {0};

    wspr(call, grid, power, symbols);

    // Print encoded packet
    std::cout << "WSPR codeblock: ";
    for (int i = 0; i < static_cast<int>(sizeof(symbols) / sizeof(*symbols)); i++) {
        if (i) {
            std::cout << ",";
        }
        std::cout << static_cast<int>(symbols[i]);
    }
    std::cout << std::endl;

    return 0;
}
#endif
