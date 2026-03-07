//  bit_utils.h
//  General-Purpose Utilities
//
//  Copyright © 2026 Christopher M. Hanson. All rights reserved.
//  See file COPYING for details.

#ifndef __BIT_UTILS__H__
#define __BIT_UTILS__H__

#include "utils_defines.h"

#include <stdint.h>
#include <stdio.h>

UTILS_HEADER_BEGIN


/*! Get the bit \a b of value \a d as an integer. */
#define BIT(d,b) (((d) & (1 << (b))) != 0)

/*! Set the bit \a b of value \a d as an integer. */
#define SET_BIT(d,b) do { d |= (1 << (b)); } while (0)

/*! Clear the bit \a b of value \a d as an integer. */
#define CLEAR_BIT(d,b) do { d &= ~(1 << (b)); } while (0);


/*! Get the high byte of the given word. */
static inline __attribute__((always_inline))
uint8_t HIGH_BYTE(uint16_t word)
{
    return (word & 0xff00) >> 8;
}

/*! Get the low byte of the given word. */
static inline __attribute__((always_inline))
uint8_t LOW_BYTE(uint16_t word)
{
    return (word & 0x00ff);
}


/*!
    Dump \a buf_size hex bytes from \a buf to \a f file, indenting each
    line by a single tab character.
 */
UTILS_EXTERN
void
dumphex(void *buf, size_t buf_size, FILE *f);


UTILS_HEADER_END

#endif /* __BIT_UTILS__H__ */
