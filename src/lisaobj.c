//  lisaobj.c
//  Part of LisaUtils.
//
//  Copyright © 2026 Christopher M. Hanson. All rights reserved.
//  See file COPYING for details.

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sysexits.h>

#include "endian_utils.h"

#include "lisa.h"


LISA_SOURCE_BEGIN


void
PrintUsage(void)
{
	fprintf(stderr, "Error!" "\n");
}


int
main(int argc, char **argv)
{
    if (argc < 2) {
        PrintUsage();
        return EX_USAGE;
    }

    lisa_objfile *ef = lisa_objfile_open(argv[1]);
    if (ef == NULL) {
        PrintUsage();
        return EX_NOINPUT;
    }

    const lisa_integer block_count = lisa_objfile_block_count(ef);
    for (lisa_integer b = 0; b < block_count; b++) {
        lisa_obj_block *block = lisa_objfile_block_at_index(ef, b);
        lisa_obj_block_dump(block);
    }

    lisa_objfile_close(ef);

    return EX_OK;
}


LISA_SOURCE_END
