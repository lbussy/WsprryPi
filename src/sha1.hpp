/**
 * @file sha1.hpp
 * @brief Generates a SHA1 hash.
 *
 * This file is part of WsprryPi, a project originally branched from
 * @threeme3's WsprryPi project (no longer on GitHub). However, now the
 * original code remains only as a memory and inspiration, and this project
 * is no longer a derivative work.
 *
 * This project is is licensed under the MIT License. See LICENSE.MIT.md
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

#ifndef SHA1_HPP
#define SHA1_HPP

#include <cstdint>
#include <cstddef>
#include <string>

/**
 * @class SHA1
 * @brief Minimal implementation of the SHA1 hashing algorithm.
 *
 * This class allows incremental hashing of data using the SHA1 algorithm.
 * It supports updates using strings or raw byte buffers, and produces a
 * 20-byte binary hash upon finalization.
 */
class SHA1
{
public:
    /**
     * @brief Constructs a new SHA1 object and initializes the internal state.
     */
    SHA1();

    /**
     * @brief Updates the SHA1 state with input from a std::string.
     *
     * This is a convenience overload that converts the string into
     * a byte buffer for hashing.
     *
     * @param s The input string to hash.
     */
    void update(const std::string &s);

    /**
     * @brief Updates the SHA1 state with raw byte data.
     *
     * This function supports streaming data into the hash. Internally,
     * it buffers input and processes it in 512-bit (64-byte) blocks.
     *
     * @param data Pointer to the input byte array.
     * @param len  Number of bytes to read from the input array.
     */
    void update(const unsigned char *data, size_t len);

    /**
     * @brief Finalizes the SHA1 computation and returns the result.
     *
     * This function applies SHA1 padding, appends the total bit length,
     * and processes any remaining input to complete the hash.
     *
     * @return std::string A binary 20-byte SHA1 digest.
     */
    std::string final();

private:
    uint32_t h0, h1, h2, h3, h4; ///< Internal SHA1 state variables.
    unsigned char buffer[64];    ///< 64-byte buffer for partial input blocks.
    size_t buffer_len;           ///< Current length of data in buffer.
    uint64_t total_len;          ///< Total input length in bytes.

    /**
     * @brief Performs a circular left rotation on a 32-bit value.
     *
     * @param value The value to rotate.
     * @param bits  Number of bits to rotate left.
     * @return uint32_t The rotated result.
     */
    static uint32_t leftrotate(uint32_t value, unsigned int bits);

    /**
     * @brief Processes a single 512-bit (64-byte) input chunk.
     *
     * This function expands the chunk into 80 words, applies the
     * SHA1 compression function, and updates the internal state.
     *
     * @param chunk A pointer to a 64-byte block of input data.
     */
    void process_chunk(const unsigned char chunk[64]);
};

#endif // SHA1_HPP
