/**
 * @file sha1.cpp
 * @brief Generates a SHA1 hash.
 *
 * This project is is licensed under the MIT License. See LICENSE.md
 * for more information.
 *
 * Copyright (C) 2023-2025 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
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

#include "sha1.hpp"

#include <algorithm>
#include <cstring>

/**
 * @brief Constructs a new SHA1 object and initializes internal state.
 *
 * Initializes the SHA1 state variables (`h0` through `h4`) with
 * the standard SHA1 constants. Also resets buffer length and total
 * length counters to zero.
 */
SHA1::SHA1()
    : h0(0x67452301), h1(0xEFCDAB89), h2(0x98BADCFE),
      h3(0x10325476), h4(0xC3D2E1F0), buffer_len(0), total_len(0)
{
}

/**
 * @brief Updates the SHA1 hash with a string input.
 *
 * This is a convenience method that forwards to the byte-based
 * update method.
 *
 * @param s The input string to hash.
 */
void SHA1::update(const std::string &s)
{
    update(reinterpret_cast<const unsigned char *>(s.c_str()), s.size());
}

/**
 * @brief Updates the SHA1 hash with raw byte data.
 *
 * Buffers the incoming data and processes it in 64-byte chunks.
 * Any remaining bytes are stored for the next update or finalization.
 *
 * @param data Pointer to the input byte array.
 * @param len  Length of the input data in bytes.
 */
void SHA1::update(const unsigned char *data, size_t len)
{
    total_len += len;
    size_t i = 0;

    if (buffer_len > 0)
    {
        size_t to_copy = std::min(len, size_t(64 - buffer_len));
        std::memcpy(buffer + buffer_len, data, to_copy);
        buffer_len += to_copy;
        i += to_copy;

        if (buffer_len == 64)
        {
            process_chunk(buffer);
            buffer_len = 0;
        }
    }

    for (; i + 64 <= len; i += 64)
    {
        process_chunk(data + i);
    }

    if (i < len)
    {
        buffer_len = len - i;
        std::memcpy(buffer, data + i, buffer_len);
    }
}

/**
 * @brief Finalizes the SHA1 hash and returns the digest.
 *
 * Performs padding and appends the total length, processes any
 * remaining data, and produces the final 20-byte digest.
 *
 * @return std::string A 20-byte binary string containing the SHA1 hash.
 */
std::string SHA1::final()
{
    uint64_t total_bits = total_len * 8;

    // Append the '1' bit.
    buffer[buffer_len++] = 0x80;

    // Pad with zeros until the length is 56 bytes mod 64.
    if (buffer_len > 56)
    {
        while (buffer_len < 64)
            buffer[buffer_len++] = 0;
        process_chunk(buffer);
        buffer_len = 0;
    }

    while (buffer_len < 56)
        buffer[buffer_len++] = 0;

    // Append the 64-bit length in big-endian format.
    for (int i = 7; i >= 0; i--)
    {
        buffer[buffer_len++] = (total_bits >> (i * 8)) & 0xFF;
    }

    process_chunk(buffer);

    unsigned char hash[20];
    auto write_uint32 = [&](uint32_t val, int offset)
    {
        hash[offset] = (val >> 24) & 0xFF;
        hash[offset + 1] = (val >> 16) & 0xFF;
        hash[offset + 2] = (val >> 8) & 0xFF;
        hash[offset + 3] = val & 0xFF;
    };

    write_uint32(h0, 0);
    write_uint32(h1, 4);
    write_uint32(h2, 8);
    write_uint32(h3, 12);
    write_uint32(h4, 16);

    return std::string(reinterpret_cast<char *>(hash), 20);
}

/**
 * @brief Performs a circular left rotation on a 32-bit integer.
 *
 * @param value The value to rotate.
 * @param bits  The number of bits to rotate by.
 * @return uint32_t The rotated result.
 */
uint32_t SHA1::leftrotate(uint32_t value, unsigned int bits)
{
    return (value << bits) | (value >> (32 - bits));
}

/**
 * @brief Processes a single 512-bit (64-byte) chunk of input data.
 *
 * Expands the chunk into 80 words, applies the SHA1 compression function,
 * and updates the internal state variables.
 *
 * @param chunk A 64-byte array representing a single input block.
 */
void SHA1::process_chunk(const unsigned char chunk[64])
{
    uint32_t w[80];

    for (int i = 0; i < 16; i++)
    {
        w[i] = (chunk[i * 4] << 24) |
               (chunk[i * 4 + 1] << 16) |
               (chunk[i * 4 + 2] << 8) |
               (chunk[i * 4 + 3]);
    }

    for (int i = 16; i < 80; i++)
    {
        uint32_t temp = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
        w[i] = leftrotate(temp, 1);
    }

    uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;

    for (int i = 0; i < 80; i++)
    {
        uint32_t f, k;

        if (i < 20)
        {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        }
        else if (i < 40)
        {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        }
        else if (i < 60)
        {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        }
        else
        {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }

        uint32_t temp = leftrotate(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = leftrotate(b, 30);
        b = a;
        a = temp;
    }

    h0 += a;
    h1 += b;
    h2 += c;
    h3 += d;
    h4 += e;
}
