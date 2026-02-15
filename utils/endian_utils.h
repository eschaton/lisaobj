//  endian_utils.h
//  General-Purpose Utilities
//
//  Copyright © 2026 Christopher M. Hanson. All rights reserved.
//  See file COPYING for details.

#ifndef __ENDIAN_UTILS__H__
#define __ENDIAN_UTILS__H__

#include "utils_defines.h"

#include <stdint.h>

UTILS_HEADER_BEGIN

#ifndef __BYTE_ORDER__
#error __BYTE_ORDER__ must be defined
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
inline
static uint16_t swapu16be(uint16_t value) {
    return (uint16_t)(  ((value & 0x00FF) << 8)
                      | ((value & 0xFF00) >> 8));
}

inline
static int16_t swap16be(int16_t value) {
    return (int16_t)swapu16be((uint16_t)value);
}

inline
static uint32_t swapu32be(uint32_t value) {
    return (  ((value & 0x000000FF) << 24)
            | ((value & 0x0000FF00) <<  8)
            | ((value & 0x00FF0000) >>  8)
            | ((value & 0xFF000000) >> 24));
}

inline
static int32_t swap32be(int32_t value) {
    return (int32_t)swapu32be((uint32_t)value);
}
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
inline
inline
static int16_t swap16be(int16_t value) {
    return value;
}

inline
static uint16_t swapu16be(uint16_t value) {
    return value;
}

inline
static int32_t swap32be(int32_t value) {
    return value;
}

inline
static uint32_t swapu32be(uint32_t value) {
    return value;
}
#else
#error PDP-11 not supported
#endif

UTILS_HEADER_END

#endif /* __ENDIAN_UTILS__H__ */
