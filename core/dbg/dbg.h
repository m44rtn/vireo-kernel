/*
MIT license
Copyright (c) 2019-2021 Maarten Vermeulen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __DBG_H__
#define __DBG_H__

#include "../screen/screen_basic.h"
#include "../kernel/info.h"
#include "debug.h"

#ifndef NDEBUG
#define dbg_assert(expression)                                          \
    if(!(expression))                                                   \
    {                                                                   \
                                                                        \
    print( "[ASSERT] Vireo:");                                                   \
    screen_basic_set_screen_color(0x03);                                \
    print_value( "%s:", (unsigned int) __FILE__);                             \
    screen_basic_set_screen_color(0x07);                                \
    print_value( "%i: Assertion Failed\n", (unsigned int) __LINE__);            \
    info_print_full_version();                                               \
                                                                        \
    while(1);                                                           \
    }                                                     
#else
#define dbg_assert(ignore) (void) 0
#endif

#endif