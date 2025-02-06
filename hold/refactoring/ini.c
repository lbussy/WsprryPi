/**
 * @file ini.c
 * @brief A simple INI file parser implementation.
 *
 * This file provides an implementation of functions for parsing INI-style configuration files.
 * The parser supports handling sections and key-value pairs, with options to handle comments,
 * inline comments, and multi-line values. It also provides support for various platforms,
 * including Windows and UNIX-like systems, through configurable visibility symbols.
 *
 * The parser functions read INI files from different sources: files, strings, or custom streams.
 * This file includes a set of configurable macros that allow customizing the behavior of the parser,
 * such as enabling multi-line value parsing, allowing UTF-8 BOM sequences, and more.
 *
 * This file is part of a project that is licensed under the GNU General Public License v3.0.
 * See the LICENSE file for details about the project's license.
 *
 * However, the inih library itself is distributed under the New BSD license:
 * Copyright (c) 2009-2020, Ben Hoyt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions, and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Ben Hoyt nor the names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BEN HOYT ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL BEN HOYT BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * For more information about inih, see: https://github.com/benhoyt/inih
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Unit test:
// gcc -Wall -Werror -g -DDEBUG_MAIN_INI -o ini ini.c
// Test command:
// ./ini wsprrypi.ini

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "ini.h"

#if !INI_USE_STACK
#if INI_CUSTOM_ALLOCATOR
#include <stddef.h>
/**
 * @brief Allocates memory for INI parser.
 *
 * @param size Size of memory to allocate.
 * @return Pointer to allocated memory.
 */
void* ini_malloc(size_t size);
void ini_free(void* ptr);
void* ini_realloc(void* ptr, size_t size);
#else
#include <stdlib.h>
#define ini_malloc malloc
#define ini_free free
#define ini_realloc realloc
#endif
#endif

#define MAX_SECTION 50 /**< Maximum length of section name. */
#define MAX_NAME 50 /**< Maximum length of key name. */

/**
 * @brief Struct to manage string parsing state for `ini_parse_string`.
 */
typedef struct {
    const char* ptr; /**< Pointer to the current position in the string. */
    size_t num_left; /**< Remaining number of characters in the string. */
} ini_parse_string_ctx;

/**
 * @brief Removes trailing whitespace from a string.
 *
 * @param s Pointer to the string.
 * @return Pointer to the modified string.
 */
static char* ini_rstrip(char* s) {
    char* p = s + strlen(s);
    while (p > s && isspace((unsigned char)(*--p))) {
        *p = '\0';
    }
    return s;
}

/**
 * @brief Skips leading whitespace in a string.
 *
 * @param s Pointer to the string.
 * @return Pointer to the first non-whitespace character.
 */
static char* ini_lskip(const char* s) {
    while (*s && isspace((unsigned char)(*s))) {
        s++;
    }
    return (char*)s;
}

/**
 * @brief Finds the first occurrence of specified characters or inline comment markers.
 *
 * @param s Pointer to the string.
 * @param chars Pointer to characters to search for.
 * @return Pointer to the first occurrence or the end of the string.
 */
static char* ini_find_chars_or_comment(const char* s, const char* chars) {
#if INI_ALLOW_INLINE_COMMENTS
    int was_space = 0;
    while (*s && (!chars || !strchr(chars, *s)) &&
           !(was_space && strchr(INI_INLINE_COMMENT_PREFIXES, *s))) {
        was_space = isspace((unsigned char)(*s));
        s++;
    }
#else
    while (*s && (!chars || !strchr(chars, *s))) {
        s++;
    }
#endif
    return (char*)s;
}

/**
 * @brief Copies a string into a buffer, ensuring null termination.
 *
 * @param dest Destination buffer.
 * @param src Source string.
 * @param size Size of the destination buffer.
 * @return Pointer to the destination buffer.
 */
static char* ini_strncpy0(char* dest, const char* src, size_t size) {
    size_t i;
    for (i = 0; i < size - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return dest;
}

/**
 * @brief Parses an INI stream using a reader function.
 *
 * @param reader Reader function to read lines from the stream.
 * @param stream Pointer to the input stream.
 * @param handler Callback function to handle parsed key-value pairs.
 * @param user User-provided data passed to the handler.
 * @return Error code or 0 on success.
 */
int ini_parse_stream(ini_reader reader, void* stream, ini_handler handler, void* user) {
#if INI_USE_STACK
    char line[INI_MAX_LINE];
    size_t max_line = INI_MAX_LINE;
#else
    char* line;
    size_t max_line = INI_INITIAL_ALLOC;
#endif
#if INI_ALLOW_REALLOC && !INI_USE_STACK
    char* new_line;
    size_t offset;
#endif
    char section[MAX_SECTION] = "";
#if INI_ALLOW_MULTILINE
    char prev_name[MAX_NAME] = "";
#endif
    char* start;
    char* end;
    char* name;
    char* value;
    int lineno = 0;
    int error = 0;

#if !INI_USE_STACK
    line = (char*)ini_malloc(INI_INITIAL_ALLOC);
    if (!line) {
        return -2;
    }
#endif

#if INI_HANDLER_LINENO
#define HANDLER(u, s, n, v) handler(u, s, n, v, lineno)
#else
#define HANDLER(u, s, n, v) handler(u, s, n, v)
#endif

    while (reader(line, (int)max_line, stream) != NULL) {
        lineno++;
        start = ini_rstrip(ini_lskip(line));
        if (strchr(INI_START_COMMENT_PREFIXES, *start)) {
            continue;
        } else if (*start == '[') {
            end = ini_find_chars_or_comment(start + 1, "]");
            if (*end == ']') {
                *end = '\0';
                ini_strncpy0(section, start + 1, sizeof(section));
#if INI_CALL_HANDLER_ON_NEW_SECTION
                if (!HANDLER(user, section, NULL, NULL) && !error) {
                    error = lineno;
                }
#endif
#if INI_ALLOW_MULTILINE
                prev_name[0] = '\0';
#endif
            } else if (!error) {
                error = lineno;
            }
        } else if (*start) {
            end = ini_find_chars_or_comment(start, "=:");
            if (*end == '=' || *end == ':') {
                *end = '\0';
                name = ini_rstrip(start);
                value = ini_lskip(end + 1);
                ini_rstrip(value);

#if INI_ALLOW_MULTILINE
                strncpy(prev_name, name, sizeof(prev_name));
#endif

                if (!HANDLER(user, section, name, value) && !error) {
                    error = lineno;
                }
            } else if (!error) {
                error = lineno;
            }
        }

#if INI_STOP_ON_FIRST_ERROR
        if (error) {
            break;
        }
#endif
    }

#if !INI_USE_STACK
    ini_free(line);
#endif

    return error;
}

/**
 * @brief Parses an INI file from a `FILE*` stream.
 *
 * @param file Pointer to the input file.
 * @param handler Callback function to handle parsed key-value pairs.
 * @param user User-provided data passed to the handler.
 * @return Error code or 0 on success.
 */
int ini_parse_file(FILE* file, ini_handler handler, void* user) {
    return ini_parse_stream((ini_reader)fgets, file, handler, user);
}

/**
 * @brief Parses an INI file.
 *
 * @param filename Path to the INI file.
 * @param handler Callback function to handle parsed key-value pairs.
 * @param user User-provided data passed to the handler.
 * @return Error code or 0 on success.
 */
int ini_parse(const char* filename, ini_handler handler, void* user) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return -1;
    }
    int error = ini_parse_file(file, handler, user);
    fclose(file);
    return error;
}

/**
 * @brief Reads the next line from a string buffer (fgets equivalent).
 *
 * @param str Buffer to store the line.
 * @param num Maximum number of characters to read.
 * @param stream Pointer to the input stream.
 * @return Pointer to the buffer or NULL on error.
 */
static char* ini_reader_string(char* str, int num, void* stream) {
    ini_parse_string_ctx* ctx = (ini_parse_string_ctx*)stream;
    const char* ctx_ptr = ctx->ptr;
    size_t ctx_num_left = ctx->num_left;
    char* strp = str;
    char c;

    if (ctx_num_left == 0 || num < 2)
        return NULL;

    while (num > 1 && ctx_num_left != 0) {
        c = *ctx_ptr++;
        ctx_num_left--;
        *strp++ = c;
        if (c == '\n')
            break;
        num--;
    }

    *strp = '\0';
    ctx->ptr = ctx_ptr;
    ctx->num_left = ctx_num_left;
    return str;
}

/**
 * @brief Parses an INI string.
 *
 * @param string Input string containing INI data.
 * @param handler Callback function to handle parsed key-value pairs.
 * @param user User-provided data passed to the handler.
 * @return Error code or 0 on success.
 */
int ini_parse_string(const char* string, ini_handler handler, void* user) {
    ini_parse_string_ctx ctx = {string, strlen(string)};
    return ini_parse_stream((ini_reader)ini_reader_string, &ctx, handler, user);
}

#ifdef DEBUG_MAIN_INI
/**
 * @brief Example handler function for INI parsing.
 *
 * @param user User-provided data (unused in this example).
 * @param section Current section name.
 * @param name Key name in the section.
 * @param value Value associated with the key.
 * @return Always returns 1 to continue parsing.
 */
static int example_handler(void* user, const char* section, const char* name, const char* value) {
    printf("Parsed -> Section: [%s], Name: '%s', Value: '%s'\n", section, name, value ? value : "(null)");
    return 1;
}

/**
 * @brief Main function for debugging the INI parser.
 *
 * Accepts a file path as an argument, parses the file, and prints its content.
 */
int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ini-file>\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    printf("Parsing INI file: %s\n\n", filename);

    int result = ini_parse(filename, example_handler, NULL);
    if (result == 0) {
        printf("\nParsing successful.\n");
    } else if (result == -1) {
        fprintf(stderr, "\nError: Unable to open file '%s'.\n", filename);
    } else if (result == -2) {
        fprintf(stderr, "\nError: Memory allocation failure.\n");
    } else {
        fprintf(stderr, "\nError: Parsing failed at line %d.\n", result);
    }

    return 0;
}
#endif
