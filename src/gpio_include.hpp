/**
 * @file gpio_include.hpp
 * @brief Manages GPIO includes and macros.
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
 
#ifndef GPIO_INCLUDE_HPP
#define GPIO_INCLUDE_HPP

#include <gpiod.hpp>

#ifdef __has_include
#  if __has_include(<gpiodcxx/version.hpp>)
#    include <gpiodcxx/version.hpp>
#  endif
#endif

#if defined(GPIOD_CXX_VERSION_MAJOR)
#  define GPIOD_API_MAJOR GPIOD_CXX_VERSION_MAJOR
#else
#  ifdef __has_include
#    if __has_include(<gpiod.h>)
extern "C" {
#include <gpiod.h>
}
#      ifdef GPIOD_VERSION_MAJOR
#        define GPIOD_API_MAJOR GPIOD_VERSION_MAJOR
#      endif
#    endif
#  endif
#endif

#ifndef GPIOD_API_MAJOR
#  define GPIOD_API_MAJOR 2   // Conservative for Trixie & future Raspberry Pi OS
#endif

#ifdef DEBUG_WSPR
#  ifndef __PRINTED_GPIOD_VERSION
#    define __PRINTED_GPIOD_VERSION 1
#    if GPIOD_API_MAJOR >= 2
#      pragma message "Compiling with libgpiod v2 support"
#    else
#      pragma message "Compiling with libgpiod v1 support"
#    endif
#  endif
#endif

#endif // GPIO_INCLUDE_HPP
