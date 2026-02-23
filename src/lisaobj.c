//  lisaobj.c
//  Part of LisaUtils.
//
//  Copyright © 2026 Christopher M. Hanson. All rights reserved.
//  See file COPYING for details.

#include <assert.h>
#include <errno.h>
#include <limits.h>
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
    lisaobj_command_extract = 1,
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
    fprintf(stderr, "  extract" "\t"   "extract" "\t\t" "extract unpacked code to files" "\n");
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
lisaobj_extract(int argc, char **argv)
{
    char current_module_name[9] = { 0 };
    char current_segment_name[9] = { 0 };
    char *first_blank;
    uint8_t *current_code = NULL;
    lisa_longint current_code_size = 0;
    lisa_MemAddr current_code_address = 0x00000000;

    const lisa_integer block_count = lisa_objfile_block_count(objfile);
    for (lisa_integer b = 0; b < block_count; b++) {
        lisa_objfile_block *block = lisa_objfile_block_at_index(objfile, b);
        lisa_objfile_content content = lisa_objfile_block_content(block);

        switch (lisa_objfile_block_type(block)) {
            case ModuleName: {
                // A ModuleName block starts a module segment, and
                // contains both the module name and the segment name.
                // These are space-padded fixed-length strings, rather
                // than length-prefixed strings, so make them into nice
                // C strings for constructing paths.

                memset(current_module_name, 0, 9);
                memcpy(current_module_name, content.ModuleName->ModuleName, 8);
                current_module_name[8] = '\0';
                first_blank = strchr(current_module_name, ' ');
                if (first_blank) *first_blank = '\0';

                memset(current_segment_name, 0, 9);
                memcpy(current_segment_name, content.ModuleName->SegmentName, 8);
                current_segment_name[8] = '\0';
                first_blank = strchr(current_segment_name, ' ');
                if (first_blank) *first_blank = '\0';
            } break;

            case PackedCode: {
                // A PackedCode block contains code for the current module,
                // as well as the start address. Save those off.

                current_code_size = content.PackedCode->csize;
                current_code_address = content.PackedCode->addr;

                assert(current_code == NULL);
                current_code = calloc(sizeof(uint8_t), (size_t)current_code_size);
                if (current_code == NULL) {
                    // TODO: Handle error.
                    assert(current_code != NULL);
                }

                uint8_t *packed_code = content.PackedCode->code;
                lisa_longint packed_code_size = lisa_objfile_block_size(block) - 12;
                // header + size + addr = 12

                int unpack_err = lisa_unpackcode(packed_code, packed_code_size,
                                                 current_code, current_code_size,
                                                 NULL);
                if (unpack_err != 0) {
                    // TODO: Handle error.
                    assert(unpack_err == 0);
                }
            } break;

            case CodeBlock: {
                // A CodeBlock contains code for the current module, as well
                // as the start address. Save those off.

                current_code_size = lisa_objfile_block_size(block) - 8;
                current_code_address = content.CodeBlock->Addr;
                // header + addr = 8

                assert(current_code == NULL);
                current_code = calloc(sizeof(uint8_t), (size_t)current_code_size);
                if (current_code == NULL) {
                    // TODO: Handle error.
                    assert(current_code != NULL);
                }

                memcpy(current_code, content.CodeBlock->code, (size_t)current_code_size);
            } break;

            case EndBlock: {
                // An EndBlock block ends the current module. Write it
                // to an appropriately-named file and clean up.

                // For the output file name, we use the input file name
                // and add -modulename[-segmentname][-address].bin so
                // it carries all of its important metadata with it.
                // (If there's no segment or the address is 0, we skip
                // those.)

                assert(current_code != NULL);

                char path[PATH_MAX];
                strlcpy(path, objfile_path, PATH_MAX);
                strlcat(path, "-", PATH_MAX);
                strlcat(path, current_module_name, PATH_MAX);

                if (current_segment_name[0] != '\0') {
                    strlcat(path, "-", PATH_MAX);
                    strlcat(path, current_segment_name, PATH_MAX);
                }

                if (current_code_address != 0) {
                    char addrbuf[10];
                    strlcat(path, "-", PATH_MAX);
                    snprintf(addrbuf, 10, "$%08x", current_code_address);
                    strlcat(path, addrbuf, PATH_MAX);
                }

                strlcat(path, ".bin", PATH_MAX);

                FILE *outfile = fopen(path, "wb");
                if (outfile == NULL) {
                    // TODO: Handle error.
                    assert(outfile != NULL);
                }

                size_t items_written = fwrite(current_code, (size_t)current_code_size, 1, outfile);
                if (items_written != 1) {
                    // TODO: Handle error.
                    assert(items_written == 1);
                }

                fclose(outfile);

                free(current_code);
                current_code = NULL;
            } break;

            default: {
                // Skip every other kind of block.
            } break;
        }
    }

    if (current_code != NULL) {
        free(current_code);
    }

    return EX_OK;
}


int
main(int argc, char **argv)
{
    program_name = argv[0];

    if (argc < 3) {
        print_usage("Insufficient arguments");
        return EX_USAGE;
    }

    objfile_path = argv[1];

    if (strcmp(argv[2], "dump") == 0) {
        command = lisaobj_command_dump;
    } else if (strcmp(argv[2], "extract") == 0) {
        command = lisaobj_command_extract;
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

        case lisaobj_command_extract:
            command_result = lisaobj_extract(command_argc, command_argv);
            break;
    }

    lisa_objfile_close(objfile);

    return command_result;
}


LISA_SOURCE_END
