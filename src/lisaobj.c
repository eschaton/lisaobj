//  lisaobj.c
//  Part of LisaUtils.
//
//  Copyright © 2026 Christopher M. Hanson. All rights reserved.
//  See file COPYING for details.

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>

#include "endian_utils.h"

#include "lisa.h"


LISA_SOURCE_BEGIN


/*! The various subcommands we support. */
enum lisaobj_command {
    lisaobj_command_dump = 0,
};
typedef enum lisaobj_command lisaobj_command;


char *program_name = NULL;
char *objfile_path = NULL;
lisa_objfile *objfile = NULL;
lisaobj_command command;


void
print_usagev(const char *errfmt, va_list args)
{
    char errstr[512];

    vsnprintf(errstr, 512, errfmt, args);

    fprintf(stderr, "An error occurred: %s" "\n", errstr);
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage:" "\n");
    fprintf(stderr, " %s object-file <command> [args]" "\n", program_name);
    fprintf(stderr, " Commands are:" "\n");
    fprintf(stderr, "  dump"    "\t\t" "dump"    "\t\t" "dump content to stdout" "\n");
}

void
print_usage(const char *errfmt, ...)
{
    va_list ap;
    va_start(ap, errfmt);
    print_usagev(errfmt, ap);
    va_end(ap);
}


int
lisaobj_dump(int argc, char **argv)
{
    const lisa_integer block_count = lisa_objfile_block_count(objfile);
    for (lisa_integer b = 0; b < block_count; b++) {
        lisa_objfile_block *block = lisa_objfile_block_at_index(objfile, b);
        lisa_obj_block_dump(block);
    }

    return EX_OK;
}


int
main(int argc, char **argv)
{
    program_name = argv[0];

    if (argc < 2) {
        print_usage("Insufficient arguments");
        return EX_USAGE;
    }

    objfile_path = argv[1];

    if (strcmp(argv[2], "dump") == 0) {
        command = lisaobj_command_dump;
    } else {
        print_usage("Unknown command: %s", argv[1]);
    }

    objfile = lisa_objfile_open(objfile_path);
    if (objfile == NULL) {
        const char *errstr = strerror(errno);
        print_usage(errstr);
        return EX_NOINPUT;
    }

    int command_result;
    int command_argc = argc - 2;
    char **command_argv = &argv[2];
    switch (command) {
        case lisaobj_command_dump:
            command_result = lisaobj_dump(command_argc, command_argv);
            break;
    }

    lisa_objfile_close(objfile);

    return command_result;
}


LISA_SOURCE_END
