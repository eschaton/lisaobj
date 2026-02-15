//  bit_utils.c
//  General-Purpose Utilities
//
//  Copyright © 2026 Christopher M. Hanson. All rights reserved.
//  See file COPYING for details.

#include "bit_utils.h"

#include <stdbool.h>

UTILS_SOURCE_BEGIN


void
dumphex(void * _Nonnull buf, size_t buf_size, FILE * _Nonnull f)
{
    uint8_t *bufu = buf;
    bool endnl = true;

    for (size_t i = 0; i < buf_size; i++) {
        if ((i % 16) == 0) {
            fprintf(f, "\t");
            if (buf_size < 65536) {
                fprintf(f, "$%04x", (int) i);
            } else {
                fprintf(f, "$%08x", (int) i);
            }
            fprintf(f, ": ");
        }

        if ((i % 16) != 15) {
            fprintf(f, "%02x" " ", bufu[i]);
            endnl = true;
        } else {
            fprintf(f, "%02x" "\n", bufu[i]);
            endnl = false;
        }
    }

    if (endnl) {
        fprintf(f, "\n");
    }
}


UTILS_SOURCE_END
