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


/*!
    Dump \a buf_size hex bytes from \a buf to \a f file, indenting each
    line by a single tab character.
 */
UTILS_EXTERN
void
dumphex(void * _Nonnull buf, size_t buf_size, FILE * _Nonnull f);


UTILS_HEADER_END

#endif /* __BIT_UTILS__H__ */
