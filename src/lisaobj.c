//  lisaobj.c
//  Part of LisaUtils.
//
//  Copyright © 2026 Christopher M. Hanson. All rights reserved.
//  See file COPYING for details.

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>

#include "endian_utils.h"

#include "lisa.h"


LISA_SOURCE_BEGIN


char *program_name = NULL;


void
print_usage(const char *errstr)
{
    fprintf(stderr, "An error occurred: %s" "\n", errstr);
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage:" "\n");
    fprintf(stderr, " %s object-file" "\n", program_name);
}


int
main(int argc, char **argv)
{
    program_name = argv[0];

    if (argc < 2) {
        print_usage("Insufficient arguments");
        return EX_USAGE;
    }

    lisa_objfile *ef = lisa_objfile_open(argv[1]);
    if (ef == NULL) {
        const char *errstr = strerror(errno);
        print_usage(errstr);
        return EX_NOINPUT;
    }

    const lisa_integer block_count = lisa_objfile_block_count(ef);
    for (lisa_integer b = 0; b < block_count; b++) {
        lisa_objfile_block *block = lisa_objfile_block_at_index(ef, b);
        lisa_obj_block_dump(block);
    }

    lisa_objfile_close(ef);

    return EX_OK;
}


LISA_SOURCE_END
