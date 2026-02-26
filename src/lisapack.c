//  lisapack.c
//  Part of LisaUtils.
//
//  Copyright © 2026 Christopher M. Hanson. All rights reserved.
//  See file COPYING for details.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>

#include "lisa.h"


LISA_SOURCE_BEGIN


int main(int argc, const char * LISA_NULLABLE argv[])
{
    int err = EX_DATAERR;
    FILE *infile = stdin;
    FILE *outfile = stdout;
    uint8_t *inbuf = NULL;
    uint8_t *outbuf = NULL;

    // Process arguments.

    if (argc < 2) {
        fprintf(stderr, "Error: Insufficient arguments" "\n");
        err = EX_USAGE;
        goto error;
    }

    const char *command_name = argv[1];

    if (argc > 2) {
        const char *infile_path = argv[2];
        if (strcmp(infile_path, "-") == 0) {
            infile = stdin;
        } else {
            infile = fopen(argv[2], "rb");
            if (infile == NULL) {
                fprintf(stderr, "Error: Cannot open input '%s'" "\n", infile_path);
                err = EX_NOINPUT;
                goto error;
            }
        }
    }

    // - is shorthand for stdout
    if (argc > 3) {
        const char *outfile_path = argv[3];
        if (strcmp(outfile_path, "-") == 0) {
            outfile = stdout;
        } else {
            outfile = fopen(argv[3], "wb");
            if (outfile == NULL) {
                fprintf(stderr, "Error: Cannot open output '%s'" "\n", outfile_path);
                err = EX_CANTCREAT;
                goto error;
            }
        }
    }

    // Set up our input buffer.

    size_t inbuf_size = 32768;
    inbuf = calloc(sizeof(uint8_t), inbuf_size);
    if (inbuf == NULL) goto error;

    // Fill our entire input buffer before behavior.

    lisa_longint inbuf_count = 0;
    do {
        uint8_t b;

        size_t items_read = fread(&b, sizeof(uint8_t), 1, infile);
        if (items_read != 1) break;

        inbuf[inbuf_count] = b;
        inbuf_count += 1;

        if (inbuf_count == (lisa_longint)inbuf_size) {
            inbuf_size += 4096;
            if (inbuf_size > INT32_MAX) goto error;
            inbuf = realloc(inbuf, inbuf_size);
            if (inbuf == NULL) goto error;
        }
    } while ((feof(infile) == 0) && (ferror(infile) == 0));

    // In the worst case, packed output can be 1.0625 times the size of
    // the input (16 input bytes passed striaght through, plus one flag
    // byte), plus 2 bytes for the footer.
    //
    // Similarly, unpacked output can be nearly twice the size of the
    // input (16 output bytes for every 8 input bytes all found in the
    // table, minus one flag byte not passed to the output).
    //
    // So, to be safe, always make the output buffer twice the size of
    // the input buffer.

    size_t outbuf_size = inbuf_size * 2;
    outbuf = calloc(sizeof(uint8_t), outbuf_size);
    if (outbuf == NULL) goto error;
    if (outbuf_size > INT32_MAX) goto error;

    lisa_longint packed_size;
    lisa_longint unpacked_size;
    size_t outbuf_count;

    if (strcmp(command_name, "pack") == 0) {
        // Pack the input to the output.

        packed_size = (lisa_longint)outbuf_size;
        unpacked_size = inbuf_count;

        int pack_err = lisa_packcode(outbuf, &packed_size,
                                     inbuf, unpacked_size, NULL);
        if (pack_err == -1) goto error;

        outbuf_count = (size_t)packed_size;
    } else if (strcmp(command_name, "unpack") == 0) {
        // Unpack the input to the output.

        packed_size = (lisa_longint)inbuf_count;
        unpacked_size = (lisa_longint)outbuf_size;

        int unpack_err = lisa_unpackcode(inbuf, packed_size,
                                         outbuf, &unpacked_size, NULL);
        if (unpack_err == -1) goto error;

        outbuf_count = (size_t)unpacked_size;
    } else {
        fprintf(stderr, "Error: Unknown command '%s'" "\n", command_name);
        err = EX_USAGE;
        goto error;
    }

    // Write the output buffer.

    size_t items_written = fwrite(outbuf, outbuf_count, 1, outfile);
    if (items_written != 1) goto error;

    // Clean up.

    fclose(infile);
    fclose(outfile);
    free(inbuf);
    free(outbuf);

    return EX_OK;

error:
    if (infile != NULL) fclose(infile);
    if (outfile != NULL) fclose(outfile);
    free(inbuf);
    free(outbuf);

    return err;
}


LISA_SOURCE_END

