/**
 * @file ini.h
 * @brief A simple INI file parser library.
 *
 * This file defines the interface for parsing INI-style configuration files.
 * The library supports reading INI files, handling sections and key-value pairs,
 * and provides an API for users to process INI files in various formats (from
 * files, strings, or custom streams).
 *
 * This library is licensed under the GNU General Public License v3.0.
 * See the LICENSE file for details about the project's license.
 *
 * However, the inih library itself is distributed under the New BSD license:
 * Copyright (c) 2009-2020, Ben Hoyt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
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

#ifndef INI_H
#define INI_H

/* Make this header file easier to include in C++ code */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/**
 * @def INI_HANDLER_LINENO
 * @brief If set to nonzero, the ini_handler callback includes a lineno parameter.
 */
#ifndef INI_HANDLER_LINENO
#define INI_HANDLER_LINENO 0
#endif

/**
 * @def INI_API
 * @brief Visibility symbols required for Windows DLLs.
 */
#ifndef INI_API
#if defined _WIN32 || defined __CYGWIN__
#	ifdef INI_SHARED_LIB
#		ifdef INI_SHARED_LIB_BUILDING
#			define INI_API __declspec(dllexport)
#		else
#			define INI_API __declspec(dllimport)
#		endif
#	else
#		define INI_API
#	endif
#else
#	if defined(__GNUC__) && __GNUC__ >= 4
#		define INI_API __attribute__ ((visibility ("default")))
#	else
#		define INI_API
#	endif
#endif
#endif

/**
 * @brief Handler function prototype for processing INI name-value pairs.
 *
 * @param user User-provided data passed to the handler.
 * @param section The name of the current section.
 * @param name The name of the key.
 * @param value The value associated with the key.
 * @return Nonzero on success, zero on failure.
 */
#if INI_HANDLER_LINENO
typedef int (*ini_handler)(void* user, const char* section, const char* name, const char* value, int lineno);
#else
typedef int (*ini_handler)(void* user, const char* section, const char* name, const char* value);
#endif

/**
 * @brief Reader function prototype for reading INI input.
 *
 * @param str Pointer to the buffer for storing the read line.
 * @param num Maximum number of characters to read.
 * @param stream Pointer to the input stream.
 * @return Pointer to the buffer or NULL on error.
 */
typedef char* (*ini_reader)(char* str, int num, void* stream);

/**
 * @brief Parses an INI-style file.
 *
 * @param filename Path to the INI file to parse.
 * @param handler Callback function to handle parsed key-value pairs.
 * @param user User-provided data passed to the handler.
 * @return 0 on success, line number of first error, -1 on file open error, or -2 on memory allocation error.
 */
INI_API int ini_parse(const char* filename, ini_handler handler, void* user);

/**
 * @brief Parses an INI-style file from a FILE* stream.
 *
 * @param file Pointer to the open file stream.
 * @param handler Callback function to handle parsed key-value pairs.
 * @param user User-provided data passed to the handler.
 * @return 0 on success, line number of first error, or -2 on memory allocation error.
 */
INI_API int ini_parse_file(FILE* file, ini_handler handler, void* user);

/**
 * @brief Parses an INI-style input from a custom stream using a reader function.
 *
 * @param reader Reader function to retrieve lines from the input stream.
 * @param stream Pointer to the input stream.
 * @param handler Callback function to handle parsed key-value pairs.
 * @param user User-provided data passed to the handler.
 * @return 0 on success, line number of first error, or -2 on memory allocation error.
 */
INI_API int ini_parse_stream(ini_reader reader, void* stream, ini_handler handler, void* user);

/**
 * @brief Parses an INI-style input from a null-terminated string.
 *
 * @param string Pointer to the input string containing INI data.
 * @param handler Callback function to handle parsed key-value pairs.
 * @param user User-provided data passed to the handler.
 * @return 0 on success, line number of first error, or -2 on memory allocation error.
 */
INI_API int ini_parse_string(const char* string, ini_handler handler, void* user);

/* Configurable behavior options. */

/**
 * @def INI_ALLOW_MULTILINE
 * @brief Allow multi-line value parsing in the style of Python's configparser.
 */
#ifndef INI_ALLOW_MULTILINE
#define INI_ALLOW_MULTILINE 1
#endif

/**
 * @def INI_ALLOW_BOM
 * @brief Allow UTF-8 BOM sequence at the start of the file.
 */
#ifndef INI_ALLOW_BOM
#define INI_ALLOW_BOM 1
#endif

/**
 * @def INI_START_COMMENT_PREFIXES
 * @brief Characters that begin a start-of-line comment.
 */
#ifndef INI_START_COMMENT_PREFIXES
#define INI_START_COMMENT_PREFIXES ";#"
#endif

/**
 * @def INI_ALLOW_INLINE_COMMENTS
 * @brief Allow inline comments following valid characters.
 */
#ifndef INI_ALLOW_INLINE_COMMENTS
#define INI_ALLOW_INLINE_COMMENTS 1
#endif

#ifndef INI_INLINE_COMMENT_PREFIXES
#define INI_INLINE_COMMENT_PREFIXES ";#"
#endif

/**
 * @def INI_USE_STACK
 * @brief Use stack for the line buffer if nonzero, otherwise use heap.
 */
#ifndef INI_USE_STACK
#define INI_USE_STACK 1
#endif

/**
 * @def INI_MAX_LINE
 * @brief Maximum line length for any line in an INI file.
 */
#ifndef INI_MAX_LINE
#define INI_MAX_LINE 200
#endif

/**
 * @def INI_ALLOW_REALLOC
 * @brief Allow heap buffer to grow dynamically via realloc().
 */
#ifndef INI_ALLOW_REALLOC
#define INI_ALLOW_REALLOC 0
#endif

/**
 * @def INI_INITIAL_ALLOC
 * @brief Initial size in bytes for heap line buffer.
 */
#ifndef INI_INITIAL_ALLOC
#define INI_INITIAL_ALLOC 200
#endif

/**
 * @def INI_STOP_ON_FIRST_ERROR
 * @brief Stop parsing on the first error if nonzero.
 */
#ifndef INI_STOP_ON_FIRST_ERROR
#define INI_STOP_ON_FIRST_ERROR 0
#endif

/**
 * @def INI_CALL_HANDLER_ON_NEW_SECTION
 * @brief Call the handler at the start of each new section.
 */
#ifndef INI_CALL_HANDLER_ON_NEW_SECTION
#define INI_CALL_HANDLER_ON_NEW_SECTION 0
#endif

/**
 * @def INI_ALLOW_NO_VALUE
 * @brief Allow names without values and call the handler with NULL for the value.
 */
#ifndef INI_ALLOW_NO_VALUE
#define INI_ALLOW_NO_VALUE 0
#endif

/**
 * @def INI_CUSTOM_ALLOCATOR
 * @brief Use custom memory allocation functions (ini_malloc, ini_free, ini_realloc).
 */
#ifndef INI_CUSTOM_ALLOCATOR
#define INI_CUSTOM_ALLOCATOR 0
#endif

#ifdef __cplusplus
}
#endif

#endif /* INI_H */
