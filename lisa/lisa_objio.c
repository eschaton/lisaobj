//  lisa_objio.c
//  Part of LisaUtils.
//
//  Copyright © 2026 Christopher M. Hanson. All rights reserved.
//  See file COPYING for details.

#include "lisa_objio.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array_utils.h"
#include "bit_utils.h"
#include "endian_utils.h"


LISA_SOURCE_BEGIN


// MARK: - Internals

struct lisa_objfile {
    void			* LISA_NULLABLE content;
    size_t			content_size;
    ptr_array		* LISA_NULLABLE blocks;
    size_t			read_offset;			//!< used while iterating blocks
};

struct lisa_objfile_block {
    lisa_objfile        	* LISA_NULLABLE objfile;    //!< backpointer into containing objfile
    lisa_obj_block_type		type;
    lisa_longint			size;						//!< total size including 4-byte header
    lisa_FileAddr			offset;						//!< offset into objfile of header
    lisa_objfile_content	content;
};


/*!
 Copy the next block from the given object file.

 - WARNING: Caller must free with `lisa_obj_block_free`.
 */
lisa_objfile_block * LISA_NULLABLE
lisa_obj_block_copy_next(lisa_objfile *of);


/*! Free the given object file block. */
void
lisa_obj_block_free(lisa_objfile_block * LISA_NULLABLE b);


/*! Swap the block's data from big-endian to native-endian. */
void
lisa_obj_block_swap(lisa_objfile_block *block);


// MARK: - Files

lisa_objfile * LISA_NULLABLE
lisa_objfile_open(const char *path)
{
    lisa_objfile *of;
    FILE *f = NULL;

    of = calloc(sizeof(lisa_objfile), 1);
    if (of == NULL) goto error;

    // Read the entire file into a contiguous buffer, to support
    // chasing of FileAddr offsets within its data structures.

    f = fopen(path, "rb");
    if (f == NULL) goto error;

    int seek1_err = fseeko(f, 0, SEEK_END);
    if (seek1_err == -1) goto error;

    off_t fs = ftello(f);
    if (fs == -1) goto error;

    of->content_size = (size_t)fs;

    int seek2_err = fseeko(f, 0, SEEK_SET);
    if (seek2_err == -1) goto error;

    of->content = calloc(of->content_size, 1);
    if (of->content == NULL) goto error;

    size_t read_items = fread(of->content, of->content_size, 1, f);
    if (read_items != 1) goto error;

    // Now create representations of all of the data structures in it.

    of->blocks = ptr_array_create(8);
    if (of->blocks == NULL) goto error;

    of->read_offset = 0;

    lisa_objfile_block *block;
    do {
        block = lisa_obj_block_copy_next(of);
        if (block) {
            ptr_array_append(of->blocks, block);
        }

        // The file will be padded to page size, so stop once a logical
        // EOF mark is encountered rather than read all the way to
        // physical EOF.
    } while ((block != NULL) && (block->type != EOFMark));

    fclose(f);
    f = NULL;

    return of;

error:
    if (f) fclose(f);
    lisa_objfile_close(of);
    return NULL;
}


void
lisa_objfile_close(lisa_objfile * LISA_NULLABLE ef)
{
    if (ef) {
        free(ef->content);

        if (ef->blocks) {
            for (size_t b = 0; b < ptr_array_count(ef->blocks); b++) {
                lisa_objfile_block *block = ptr_array_item_at_index(ef->blocks, b);
                lisa_obj_block_free(block);
            }
            ptr_array_free(ef->blocks);
        }

        free(ef);
    }
}


lisa_integer
lisa_objfile_block_count(lisa_objfile *of)
{
    size_t count = ptr_array_count(of->blocks);
    assert(count <= INT16_MAX);

    return (lisa_integer)count;
}


lisa_objfile_block *
lisa_objfile_block_at_index(lisa_objfile *of, lisa_integer idx)
{
    return ptr_array_item_at_index(of->blocks, (size_t)idx);
}


// MARK: - Blocks

lisa_obj_block_type
lisa_objfile_block_type(lisa_objfile_block *block)
{
    return block->type;
}

lisa_longint
lisa_objfile_block_size(lisa_objfile_block *block)
{
    return block->size;
}

lisa_objfile_content
lisa_objfile_block_content(lisa_objfile_block *block)
{
    return block->content;
}

const char *
lisa_obj_block_type_string(lisa_obj_block_type t)
{
    switch (t) {
        case ModuleName:		return "ModuleName";
        case EndBlock:			return "EndBlock";
        case EntryPoint:		return "EntryPoint";
        case External:			return "External";
        case StartAddress:		return "StartAddress";
        case CodeBlock:			return "CodeBlock";
        case Relocation:		return "Relocation";
        case CommonRelocation:	return "CommonRelocation";
        case ShortExternal:		return "ShortExternal";
        case OldExecutable:		return "OldExecutable";
        case UnitBlock:			return "UnitBlock";
        case PhysicalExec:		return "PhysicalExec";
        case Executable:		return "Executable";
        case VersionCtrl:		return "VersionCtrl";
        case SegmentTable:		return "SegmentTable";
        case UnitTable:			return "UnitTable";
        case SegLocation:		return "SegLocation";
        case UnitLocation:		return "UnitLocation";
        case StringBlock:		return "StringBlock";
        case PackedCode:		return "PackedCode";
        case PackTable:			return "PackTable";
        case OSData:			return "OSData";
        case EOFMark:			return "EOFMark";

        default: {
            static char buf[32];
            snprintf(buf, 32, "Unknown($%02x)", t);
            return buf;
        } break;
    }
}


const char *
lisa_UnitType_string(lisa_UnitType t)
{
    switch (t) {
        case RegularUnit:	return "Regular";
        case IntrinsicUnit:	return "Intrinsic";
        case SharedUnit:	return "Shared";

        default: {
            static char buf[32];
            snprintf(buf, 32, "Unknown($%04x)", t);
            return buf;
        } break;
    }
}


void
lisa_obj_block_free(lisa_objfile_block * LISA_NULLABLE b)
{
    free(b);
}


int
lisa_obj_read_raw(lisa_objfile *of, void *buf, size_t size)
{
    uint8_t *content = of->content;

    if ((of->read_offset + size) > of->content_size) {
        errno = EIO;
        return -1;
    }

    memcpy(buf, &content[of->read_offset], size);

    of->read_offset += size;

    return 0;
}

lisa_objfile_block * LISA_NULLABLE
lisa_obj_block_copy_next(lisa_objfile *of)
{
    lisa_objfile_block *block = NULL;
    uint8_t buf[4];

    // Save the pre-read block offset.

    size_t offset = of->read_offset;
    if (offset > INT32_MAX) goto error;

    // Read the block type and size.

    int read_err = lisa_obj_read_raw(of, buf, 4);
    if (read_err == -1) goto error;

    // Make room for new processed block.

    block = calloc(sizeof(lisa_objfile_block), 1);
    if (block == NULL) goto error;

    block->objfile = of;
    block->offset = (lisa_FileAddr)offset;
    block->type = (lisa_obj_block_type)buf[0];
    block->size = ((buf[1] << 16) | (buf[2] << 8) | (buf[3] << 0));

    // size includes header but data does not
    uint8_t *content_bytes = of->content;
    block->content.data = &content_bytes[of->read_offset];
    of->read_offset += (size_t) block->size - 4;

    // Swap the block data if necessary.

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    lisa_obj_block_swap(block);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    // No swapping necessary.
#else
#error PDP-11 not supported
#endif

    return block;

error:
    lisa_obj_block_free(block);
    return NULL;
}


void
lisa_objfile_copy_pstring_at_offset(lisa_objfile *of,
                                    char *cstr,
                                    lisa_FileAddr offset)
{
    char *content_chars = of->content;
    char *pstr_in_block = &content_chars[offset];
    size_t len = (size_t)pstr_in_block[0];
    memcpy(cstr, &pstr_in_block[1], len);
    cstr[len] = '\0';
}


lisa_JTSegVariantTable *
lisa_Executable_JTSegVariantTable(lisa_Executable *executable)
{
    uint8_t *raw = (uint8_t *)executable;
    return (lisa_JTSegVariantTable *)&raw[sizeof(lisa_Executable)];
}


lisa_JTVariantTable *
lisa_Executable_JTVariantTable(lisa_Executable *executable)
{
    lisa_JTSegVariantTable *jtSegVariantTable = lisa_Executable_JTSegVariantTable(executable);
    lisa_integer numSegs = jtSegVariantTable->numSegs; // NOTE: MUST be swapped already!
    uint8_t *raw = (uint8_t *)&jtSegVariantTable->variants[numSegs];
    return (lisa_JTVariantTable *)raw;
}


void
lisa_obj_block_swap(lisa_objfile_block *block)
{
    switch (lisa_objfile_block_type(block)) {
        case ModuleName: {
            lisa_ModuleName *modulename = block->content.ModuleName;
            modulename->CSize = swap32be(modulename->CSize);
        } break;

        case EndBlock: {
            lisa_EndBlock *endblock = block->content.EndBlock;
            endblock->CSize = swap32be(endblock->CSize);
        } break;

        case EntryPoint: {
            lisa_EntryPoint *entrypoint = block->content.EntryPoint;
            entrypoint->Loc = swap32be(entrypoint->Loc);
        } break;

        case External: {
            lisa_External *external = block->content.External;
            lisa_integer count = (lisa_integer)(((size_t)block->size - 12) / sizeof(lisa_SegAddr));
            for (lisa_integer i = 0; i < count; i++) {
                external->Ref[i] = swap32be(external->Ref[i]);
            }
        } break;

        case StartAddress: {
            lisa_StartAddress *startaddress = block->content.StartAddress;
            startaddress->Start = swap32be(startaddress->Start);
            startaddress->GSize = swap32be(startaddress->GSize);
        } break;

        case CodeBlock: {
            lisa_CodeBlock *codeblock = block->content.CodeBlock;
            codeblock->Addr = swap32be(codeblock->Addr);
        } break;

        case Relocation: {
            lisa_Relocation *relocation = block->content.Relocation;
            lisa_integer count = (lisa_integer)(((size_t)block->size - 4) / sizeof(lisa_SegAddr));
            for (lisa_integer i = 0; i < count; i++) {
                relocation->Ref[i] = swap32be(relocation->Ref[i]);
            }
        } break;

        case CommonRelocation: {
            lisa_CommonRelocation *commonrelocation = block->content.CommonRelocation;
            lisa_integer count = (lisa_integer)(((size_t)block->size - 12) / sizeof(lisa_SegAddr));
            for (lisa_integer i = 0; i < count; i++) {
                commonrelocation->Ref[i] = swap32be(commonrelocation->Ref[i]);
            }
        } break;

        case ShortExternal: {
            lisa_ShortExternal *shortexternal = block->content.ShortExternal;
            lisa_integer count = (lisa_integer)(((size_t)block->size - 20) / sizeof(lisa_integer));
            for (lisa_integer i = 0; i < count; i++) {
                shortexternal->ShortRef[i] = swap16be(shortexternal->ShortRef[i]);
            }
        } break;

        case OldExecutable: {
            // TODO: Swap OldExecutable
        } break;

        case UnitBlock: {
            lisa_UnitBlock *unitblock = block->content.UnitBlock;
            unitblock->CodeAddr = swap32be(unitblock->CodeAddr);
            unitblock->TextAddr = swap32be(unitblock->TextAddr);
            unitblock->TextSize = swap32be(unitblock->TextSize);
            unitblock->GlobalSize = swap32be(unitblock->GlobalSize);
            unitblock->UnitType = swap16be(unitblock->UnitType);
        } break;

        case PhysicalExec: {
            // TODO: Swap PhysicalExec
        } break;

        case Executable: {
            lisa_Executable *executable = block->content.Executable;
            executable->JTLaddr = swap32be(executable->JTLaddr);
            executable->JTSize = swap32be(executable->JTSize);
            executable->DataSize = swap32be(executable->DataSize);
            executable->MainSize = swap32be(executable->MainSize);
            executable->JTSegDelta = swap32be(executable->JTSegDelta);
            executable->StkSegDelta = swap32be(executable->StkSegDelta);
            executable->DynStack = swap32be(executable->DynStack);
            executable->MaxStack = swap32be(executable->MaxStack);
            executable->MinHeap = swap32be(executable->MinHeap);
            executable->MaxHeap = swap32be(executable->MaxHeap);

            lisa_JTSegVariantTable *jtSegVariantTable = lisa_Executable_JTSegVariantTable(executable);
            jtSegVariantTable->numSegs = swap16be(jtSegVariantTable->numSegs);
            for (lisa_integer i = 0; i < jtSegVariantTable->numSegs; i++) {
                jtSegVariantTable->variants[i].SegmentAddr = swap32be(jtSegVariantTable->variants[i].SegmentAddr);
                jtSegVariantTable->variants[i].SizePacked = swap16be(jtSegVariantTable->variants[i].SizePacked);
                jtSegVariantTable->variants[i].SizeUnpacked = swap16be(jtSegVariantTable->variants[i].SizeUnpacked);
                jtSegVariantTable->variants[i].MemLoc = swap32be(jtSegVariantTable->variants[i].MemLoc);
            }

            lisa_JTVariantTable *jtVariantTable = lisa_Executable_JTVariantTable(executable);
            jtVariantTable->numDescriptors = swap16be(jtVariantTable->numDescriptors);
            for (lisa_integer i = 0; i < jtVariantTable->numDescriptors; i++) {
                jtVariantTable->variants[i].JumpL = swap16be(jtVariantTable->variants[i].JumpL);
                jtVariantTable->variants[i].AbsAddr = swap32be(jtVariantTable->variants[i].AbsAddr);
            }
        } break;

        case VersionCtrl: {
            lisa_VersionCtrl *versionctrl = block->content.VersionCtrl;
            versionctrl->sysNum = swap32be(versionctrl->sysNum);
            versionctrl->minSys = swap32be(versionctrl->minSys);
            versionctrl->maxSys = swap32be(versionctrl->maxSys);
            versionctrl->Reserv1 = swap32be(versionctrl->Reserv1);
            versionctrl->Reserv2 = swap32be(versionctrl->Reserv2);
            versionctrl->Reserv3 = swap32be(versionctrl->Reserv3);
        } break;

        case SegmentTable: {
            lisa_SegmentTable *segmenttable = block->content.SegmentTable;
            segmenttable->nSegments = swap16be(segmenttable->nSegments);
            for (lisa_integer i = 0; i < segmenttable->nSegments; i++) {
                segmenttable->variants[i].SegNumber = swap16be(segmenttable->variants[i].SegNumber);
                segmenttable->variants[i].Version1 = swap32be(segmenttable->variants[i].Version1);
                segmenttable->variants[i].Version2 = swap32be(segmenttable->variants[i].Version2);
            }
        } break;

        case UnitTable: {
            lisa_UnitTable *unittable = block->content.UnitTable;
            unittable->nUnits = swap16be(unittable->nUnits);
            unittable->maxunit = swap16be(unittable->maxunit);

            for (lisa_integer i = 0; i < unittable->nUnits; i++) {
                unittable->variants[i].UnitNumber = swap16be(unittable->variants[i].UnitNumber);
                unittable->variants[i].UnitType = swap16be(unittable->variants[i].UnitType);
            }
        } break;

        case SegLocation: {
            lisa_SegLocation *seglocation = block->content.SegLocation;
            seglocation->nSegments = swap16be(seglocation->nSegments);
            for (lisa_integer i = 0; i < seglocation->nSegments; i++) {
                seglocation->variants[i].Version1 = swap32be(seglocation->variants[i].Version1);
                seglocation->variants[i].Version2 = swap32be(seglocation->variants[i].Version2);
                seglocation->variants[i].FileNumber = swap16be(seglocation->variants[i].FileNumber);
                seglocation->variants[i].FileLocation = swap32be(seglocation->variants[i].FileLocation);
                seglocation->variants[i].SizePacked = swap16be(seglocation->variants[i].SizePacked);
                seglocation->variants[i].SizeUnpacked = swap16be(seglocation->variants[i].SizeUnpacked);
            }
        } break;

        case UnitLocation: {
            lisa_UnitLocation *unitlocation = block->content.UnitLocation;
            unitlocation->nUnits = swap16be(unitlocation->nUnits);
            for (lisa_integer i = 0; i < unitlocation->nUnits; i++) {
                unitlocation->variants[i].UnitNumber = swap16be(unitlocation->variants[i].UnitNumber);
                unitlocation->variants[i].DataSize = swap32be(unitlocation->variants[i].DataSize);
            }
        } break;

        case StringBlock: {
            lisa_StringBlock *stringblock = block->content.StringBlock;
            stringblock->nStrings = swap16be(stringblock->nStrings);
            for (lisa_integer i = 0; i < stringblock->nStrings; i++) {
                stringblock->variants[i].FileNumber = swap16be(stringblock->variants[i].FileNumber);
                stringblock->variants[i].NameAddr = swap32be(stringblock->variants[i].NameAddr);
            }
        } break;

        case PackedCode: {
            lisa_PackedCode *packedcode = block->content.PackedCode;
            packedcode->addr = swap32be(packedcode->addr);
            packedcode->csize = swap32be(packedcode->csize);
        } break;

        case PackTable: {
            lisa_PackTable *packtable = block->content.PackTable;
            packtable->packversion = swap32be(packtable->packversion);
        } break;

        case OSData: {
            // Haven't been able to find any information about what
            // might be in this.
        } break;

        case EOFMark: {
        } break;
    }
}


void
lisa_obj_block_dump(lisa_objfile_block *block)
{
    // Print header info.
    fprintf(stdout, "%s ($%02X), offset %u, %u total bytes" "\n",
            lisa_obj_block_type_string(block->type), block->type,
            block->offset, block->size);

    // Print block-specific info.
    switch (lisa_objfile_block_type(block)) {
        case ModuleName: {
            char buf[9];

            lisa_ModuleName *modulename = block->content.ModuleName;
            memset(buf, 0, 9);
            memcpy(buf, modulename->ModuleName, 8);
            fprintf(stdout, "\t" "ModuleName: '%s'" "\n", buf);
            memset(buf, 0, 9);
            memcpy(buf, modulename->SegmentName, 8);
            fprintf(stdout, "\t" "SegmentName: '%s'" "\n", buf);
            fprintf(stdout, "\t" "CSize: %d" "\n", modulename->CSize);
        } break;

        case EndBlock: {
            lisa_EndBlock *endblock = block->content.EndBlock;
            fprintf(stdout, "\t" "CSize: %d" "\n", endblock->CSize);
        } break;

        case EntryPoint: {
            char buf[9];

            lisa_EntryPoint *entrypoint = block->content.EntryPoint;
            memset(buf, 0, 9);
            memcpy(buf, entrypoint->LinkName, 8);
            fprintf(stdout, "\t" "LinkName: '%s'" "\n", buf);
            memset(buf, 0, 9);
            memcpy(buf, entrypoint->UserName, 8);
            fprintf(stdout, "\t" "UserName: '%s'" "\n", buf);
            fprintf(stdout, "\t" "Loc: $%08x" "\n", entrypoint->Loc);
        } break;

        case External: {
            char buf[9];

            lisa_External *external = block->content.External;
            memset(buf, 0, 9);
            memcpy(buf, external->LinkName, 8);
            fprintf(stdout, "\t" "LinkName: '%s'" "\n", buf);
            memset(buf, 0, 9);
            memcpy(buf, external->UserName, 8);
            fprintf(stdout, "\t" "UserName: '%s'" "\n", buf);

            lisa_integer count = (lisa_integer)(((size_t)block->size - 12) / sizeof(lisa_SegAddr));
            fprintf(stdout, "\t" "nRefs: %d" "\n", count);

            for (lisa_integer i = 0; i < count; i++) {
                fprintf(stdout, "\t\t" "Ref[%d]: %d" "\n", i, external->Ref[i]);
            }
        } break;

        case StartAddress: {
            lisa_StartAddress *startaddress = block->content.StartAddress;
            fprintf(stdout, "\t" "Start: $%08x" "\n", startaddress->Start);
            fprintf(stdout, "\t" "GSize: %d" "\n", startaddress->GSize);
        } break;

        case CodeBlock: {
            lisa_CodeBlock *codeblock = block->content.CodeBlock;
            fprintf(stdout, "\t" "Addr: $%08x" "\n", codeblock->Addr);

            lisa_longint size = block->size - 8; // header + Addr = 8
            uint8_t *code = codeblock->code;

            dumphex(code, (size_t) size, stdout);
        } break;

        case Relocation: {
            lisa_Relocation *relocation = block->content.Relocation;
            lisa_integer count = (lisa_integer)(((size_t)block->size - 4) / sizeof(lisa_SegAddr));
            fprintf(stdout, "\t" "nRefs: %d" "\n", count);

            for (lisa_integer i = 0; i < count; i++) {
                fprintf(stdout, "\t\t" "Ref[%d]: %d" "\n", i, relocation->Ref[i]);
            }
        } break;

        case CommonRelocation: {
            char buf[9];

            lisa_CommonRelocation *commonrelocation = block->content.CommonRelocation;
            memset(buf, 0, 9);
            memcpy(buf, commonrelocation->CommonName, 8);
            fprintf(stdout, "\t" "CommonName: '%s'" "\n", buf);

            lisa_integer count = (lisa_integer)(((size_t)block->size - 12) / sizeof(lisa_SegAddr));
            fprintf(stdout, "\t" "nRefs: %d" "\n", count);

            for (lisa_integer i = 0; i < count; i++) {
                fprintf(stdout, "\t\t" "Ref[%d]: %d" "\n", i, commonrelocation->Ref[i]);
            }
        } break;

        case ShortExternal: {
            char buf[9];

            lisa_ShortExternal *shortexternal = block->content.ShortExternal;
            memset(buf, 0, 9);
            memcpy(buf, shortexternal->LinkName, 8);
            fprintf(stdout, "\t" "LinkName: '%s'" "\n", buf);
            memset(buf, 0, 9);
            memcpy(buf, shortexternal->UserName, 8);
            fprintf(stdout, "\t" "UserName: '%s'" "\n", buf);

            lisa_integer count = (lisa_integer)(((size_t)block->size - 20) / sizeof(lisa_integer));
            fprintf(stdout, "\t" "nShortRefs: %d" "\n", count);

            for (lisa_integer i = 0; i < count; i++) {
                fprintf(stdout, "\t\t" "ShortRef[%d]: %d" "\n", i, shortexternal->ShortRef[i]);
            }
        } break;

        case OldExecutable: {
            // TODO: Dump OldExecutable
            fprintf(stdout, "\t" "UNIMPLEMENTED" "\n");
        } break;

        case UnitBlock: {
            char buf[9];

            lisa_UnitBlock *unitblock = block->content.UnitBlock;
            memset(buf, 0, 9);
            memcpy(buf, unitblock->UnitName, 8);
            fprintf(stdout, "\t" "UnitName: '%s'" "\n", buf);
            fprintf(stdout, "\t" "CodeAddr: $%08x" "\n", unitblock->CodeAddr);
            fprintf(stdout, "\t" "TextAddr: $%08x" "\n", unitblock->TextAddr);
            fprintf(stdout, "\t" "TextSize: %d" "\n", unitblock->TextSize);
            fprintf(stdout, "\t" "GlobalSize: %d" "\n", unitblock->GlobalSize);
            fprintf(stdout, "\t" "UnitType: %s" "\n", lisa_UnitType_string(unitblock->UnitType));
        } break;

        case PhysicalExec: {
            // TODO: Dump PhysicalExec
            fprintf(stdout, "\t" "UNIMPLEMENTED" "\n");
        } break;

        case Executable: {
            lisa_Executable *executable = block->content.Executable;
            fprintf(stdout, "\t" "JTLaddr: $%08x" "\n", executable->JTLaddr);
            fprintf(stdout, "\t" "JTSize: %d" "\n", executable->JTSize);
            fprintf(stdout, "\t" "DataSize: %d" "\n", executable->DataSize);
            fprintf(stdout, "\t" "MainSize: %d" "\n", executable->MainSize);
            fprintf(stdout, "\t" "JTSegDelta: %d" "\n", executable->JTSegDelta);
            fprintf(stdout, "\t" "StkSegDelta: %d" "\n", executable->StkSegDelta);
            fprintf(stdout, "\t" "DynStack: %d" "\n", executable->DynStack);
            fprintf(stdout, "\t" "MaxStack: %d" "\n", executable->MaxStack);
            fprintf(stdout, "\t" "MinHeap: %d" "\n", executable->MinHeap);
            fprintf(stdout, "\t" "MaxHeap: %d" "\n", executable->MaxHeap);

            lisa_JTSegVariantTable *jtSegVariantTable = lisa_Executable_JTSegVariantTable(executable);
            fprintf(stdout, "\t" "numSegs: %d" "\n", jtSegVariantTable->numSegs);
            for (lisa_integer i = 0; i < jtSegVariantTable->numSegs; i++) {
                fprintf(stdout, "\t" "[%d]{" "\n", i);
                fprintf(stdout, "\t\t" "SegmentAddr: %d" "\n", jtSegVariantTable->variants[i].SegmentAddr);
                fprintf(stdout, "\t\t" "SizePacked: %d" "\n", jtSegVariantTable->variants[i].SizePacked);
                fprintf(stdout, "\t\t" "SizeUnpacked: %d" "\n", jtSegVariantTable->variants[i].SizeUnpacked);
                fprintf(stdout, "\t\t" "MemLoc: $%08x" "\n", jtSegVariantTable->variants[i].MemLoc);
                fprintf(stdout, "\t" "}" "\n");
            }

            lisa_JTVariantTable *jtVariantTable = lisa_Executable_JTVariantTable(executable);
            fprintf(stdout, "\t" "numDescriptors: %d" "\n", jtVariantTable->numDescriptors);
            for (lisa_integer i = 0; i < jtVariantTable->numDescriptors; i++) {
                fprintf(stdout, "\t" "[%d]{" "\n", i);
                fprintf(stdout, "\t\t" "JumpL: $%04x" "\n", jtVariantTable->variants[i].JumpL);
                fprintf(stdout, "\t\t" "AbsAddr: $%08x" "\n", jtVariantTable->variants[i].AbsAddr);
                fprintf(stdout, "\t" "}" "\n");
            }
        } break;

        case VersionCtrl: {
            lisa_VersionCtrl *versionctrl = block->content.VersionCtrl;
            fprintf(stdout, "\t" "sysNum: $%08x" "\n", versionctrl->sysNum);
            fprintf(stdout, "\t" "minSys: $%08x" "\n", versionctrl->minSys);
            fprintf(stdout, "\t" "maxSys: $%08x" "\n", versionctrl->maxSys);
            fprintf(stdout, "\t" "Reserv1: $%08x" "\n", versionctrl->Reserv1);
            fprintf(stdout, "\t" "Reserv2: $%08x" "\n", versionctrl->Reserv2);
            fprintf(stdout, "\t" "Reserv3: $%08x" "\n", versionctrl->Reserv3);
        } break;

        case SegmentTable: {
            char buf[9];

            lisa_SegmentTable *segmenttable = block->content.SegmentTable;
            fprintf(stdout, "\t" "nSegments: %d" "\n", segmenttable->nSegments);

            for (lisa_integer i = 0; i < segmenttable->nSegments; i++) {
                fprintf(stdout, "\t" "[%d]{" "\n", i);
                memset(buf, 0, 9);
                memcpy(buf, segmenttable->variants[i].SegName, 8);
                fprintf(stdout, "\t\t" "SegName: '%s'" "\n", buf);
                fprintf(stdout, "\t\t" "SegNumber: %d" "\n", segmenttable->variants[i].SegNumber);
                fprintf(stdout, "\t\t" "Version1: $%08x" "\n", segmenttable->variants[i].Version1);
                fprintf(stdout, "\t\t" "Version2: $%08x" "\n", segmenttable->variants[i].Version2);
                fprintf(stdout, "\t" "}" "\n");
            }
        } break;

        case UnitTable: {
            char buf[9];

            lisa_UnitTable *unittable = block->content.UnitTable;
            fprintf(stdout, "\t" "nUnits: %d" "\n", unittable->nUnits);
            fprintf(stdout, "\t" "maxunit: %d" "\n", unittable->maxunit);

            for (lisa_integer i = 0; i < unittable->nUnits; i++) {
                fprintf(stdout, "\t" "[%d]{" "\n", i);
                memset(buf, 0, 9);
                memcpy(buf, unittable->variants[i].UnitName, 8);
                fprintf(stdout, "\t\t" "UnitName: '%s'" "\n", buf);
                fprintf(stdout, "\t\t" "UnitNumber: %d" "\n", unittable->variants[i].UnitNumber);
                fprintf(stdout, "\t\t" "UnitType: %s" "\n", lisa_UnitType_string(unittable->variants[i].UnitType));
                fprintf(stdout, "\t" "}" "\n");
            }
        } break;

        case SegLocation: {
            char buf[9];

            lisa_SegLocation *seglocation = block->content.SegLocation;
            fprintf(stdout, "\t" "nSegments: %d" "\n", seglocation->nSegments);

            for (lisa_integer i = 0; i < seglocation->nSegments; i++) {
                fprintf(stdout, "\t" "[%d]{" "\n", i);
                memset(buf, 0, 9);
                memcpy(buf, seglocation->variants[i].SegName, 8);
                fprintf(stdout, "\t\t" "SegName: '%s'" "\n", buf);
                fprintf(stdout, "\t\t" "Version1: $%08x" "\n", seglocation->variants[i].Version1);
                fprintf(stdout, "\t\t" "Version2: $%08x" "\n", seglocation->variants[i].Version2);
                fprintf(stdout, "\t\t" "FileNumber: %d" "\n", seglocation->variants[i].FileNumber);
                fprintf(stdout, "\t\t" "FileLocation: %d" "\n", seglocation->variants[i].FileLocation);
                fprintf(stdout, "\t\t" "SizePacked: %d" "\n", seglocation->variants[i].SizePacked);
                fprintf(stdout, "\t\t" "SizeUnpacked: %d" "\n", seglocation->variants[i].SizeUnpacked);
                fprintf(stdout, "\t" "}" "\n");
            }
        } break;

        case UnitLocation: {
            char buf[9];

            lisa_UnitLocation *unitlocation = block->content.UnitLocation;
            fprintf(stdout, "\t" "nUnits: %d" "\n", unitlocation->nUnits);

            for (lisa_integer i = 0; i < unitlocation->nUnits; i++) {
                fprintf(stdout, "\t" "[%d]{" "\n", i);
                memset(buf, 0, 9);
                memcpy(buf, unitlocation->variants[i].UnitName, 8);
                fprintf(stdout, "\t\t" "UnitName: '%s'" "\n", buf);
                fprintf(stdout, "\t\t" "UnitNumber: %d" "\n", unitlocation->variants[i].UnitNumber);
                fprintf(stdout, "\t\t" "FileNumber: %d" "\n", unitlocation->variants[i].FileNumber);
                fprintf(stdout, "\t\t" "UnitType: %s" "\n", lisa_UnitType_string(unitlocation->variants[i].UnitType));
                fprintf(stdout, "\t\t" "DataSize: %d" "\n", unitlocation->variants[i].DataSize);
                fprintf(stdout, "\t" "}" "\n");
            }
        } break;

        case StringBlock: {
            lisa_StringBlock *stringblock = block->content.StringBlock;
            fprintf(stdout, "\t" "nStrings: %d" "\n", stringblock->nStrings);

            for (lisa_integer i = 0; i < stringblock->nStrings; i++) {
                fprintf(stdout, "\t" "[%d]{" "\n", i);
                fprintf(stdout, "\t\t" "FileNumber: %d" "\n", stringblock->variants[i].FileNumber);
                fprintf(stdout, "\t\t" "NameAddr: %d" "\n", stringblock->variants[i].NameAddr);

                char str[256];
                lisa_objfile_copy_pstring_at_offset(block->objfile, str, stringblock->variants[i].NameAddr);

                fprintf(stdout, "\t\t" "Name: '%s'" "\n", str);

                fprintf(stdout, "\t" "}" "\n");
            }
        } break;

        case PackedCode: {
            lisa_PackedCode *packedcode = block->content.PackedCode;
            fprintf(stdout, "\t" "addr: $%08x" "\n", packedcode->addr);
            fprintf(stdout, "\t" "csize: %d" "\n", packedcode->csize);

            lisa_longint packed_size = block->size - 12; // header + addr + csize = 12
            uint8_t *packed = packedcode->code;

            lisa_longint unpacked_size = packedcode->csize;
            uint8_t *unpacked = calloc(sizeof(uint8_t), (size_t)unpacked_size);
            if (unpacked) {
                int unpack_err = lisa_unpackcode(packed, packed_size,
                                                 unpacked, unpacked_size,
                                                 NULL);
                if (unpack_err == 0) {
                    dumphex(unpacked, (size_t) unpacked_size, stdout);
                } else {
                    fprintf(stderr, "unpacking error %d" "\n", unpack_err);
                }
                free(unpacked);
            } else {
                fprintf(stderr, "error allocating storage for unpacked code" "\n");
            }
        } break;

        case PackTable: {
            lisa_PackTable *packtable = block->content.PackTable;
            fprintf(stdout, "\t" "packversion: %d" "\n", packtable->packversion);

            if (packtable->packversion == 1) {
                dumphex(packtable->words, sizeof(lisa_integer) * 256, stdout);
            } else {
                dumphex(packtable->words, (size_t)block->size - 8, stdout);
            }
        } break;

        case OSData: {
            lisa_OSData *osdata = block->content.OSData;
            dumphex(osdata->bitmap, 16, stdout);
        } break;

        case EOFMark: {
        } break;
    }
}


/*!
	Lisa code unpacking data table, aka `SYSTEM.UNPACK`.
 */
const uint16_t lisa_unpackcode_table[256] = {
    0x0000, 0x2020, 0xC1FC, 0x670E, 0x007A, 0x422D, 0x2070, 0x226E,
    0x0000, 0x504F, 0x0005, 0x2006, 0xFFE2, 0x0006, 0x2007, 0x5340,
    0xE340, 0xFFE4, 0xFFEA, 0x2F0E, 0x18F0, 0x6702, 0x3F2C, 0x0030,
    0x6000, 0x6008, 0x8001, 0x3200, 0x0022, 0x2E9F, 0x000A, 0x2045,
    0x102C, 0x205F, 0x0016, 0x102E, 0x0010, 0x6E12, 0x3F2D, 0x1F3C,
    0x4A50, 0x0018, 0x0008, 0x2F3C, 0x3F00, 0x001A, 0x6700, 0x3D40,
    0x486D, 0x0034, 0x6608, 0xA07C, 0x422E, 0xFFD0, 0x0C6E, 0x426E,
    0xFFD2, 0x4A6E, 0xFFD4, 0x1028, 0x22D8, 0x2D5F, 0xFFE6, 0xDEFC,
    0x001C, 0x41EE, 0xB06E, 0x2F2B, 0xBE6E, 0x00FF, 0x7E01, 0x6706,
    0x670A, 0x2068, 0xFFFA, 0x2F28, 0x4250, 0x6710, 0x2D40, 0x302C,
    0x6708, 0x2F2C, 0xFFFC, 0x30BC, 0x4E5E, 0x201F, 0x2D48, 0x2F0B,
    0x48C0, 0x302E, 0xFFCA, 0x2F10, 0x6600, 0x426C, 0x41ED, 0x0C47,
    0x2053, 0x6EFA, 0xFFEC, 0x2F08, 0xFFF6, 0x0014, 0x206C, 0x0001,
    0x3D6E, 0x1F2E, 0x000E, 0x486E, 0x6002, 0x0024, 0x2050, 0x0098,
    0xFFE8, 0x00C2, 0x3F28, 0x3091, 0x2046, 0xC001, 0x4CDF, 0x0009,
    0x4441, 0x4247, 0x266D, 0x0A3C, 0x3F07, 0x002C, 0x302D, 0x4868,
    0x56C0, 0x20D9, 0xA08C, 0x4A10, 0xFFDA, 0xFFF2, 0x286E, 0xFFEE,
    0x2F2D, 0x6604, 0x6004, 0xFFFF, 0xFFC0, 0x3F2E, 0x670C, 0x2F0C,
    0x0002, 0x2F00, 0x2047, 0x0020, 0x000C, 0x000F, 0x0003, 0x3D7C,
    0xA0AC, 0x5247, 0xA0AE, 0x3F3C, 0x600C, 0x001E, 0xA0C0, 0x0012,
    0x202E, 0x1D7C, 0x0C2C, 0x41E8, 0xA022, 0x0032, 0xFFDC, 0xFFF4,
    0x0130, 0x266E, 0xFFDE, 0x4EBA, 0x4E75, 0xA028, 0x3940, 0x7000,
    0xA030, 0xFFF8, 0x2F07, 0xFFC4, 0xA034, 0x486C, 0x6712, 0x56C1,
    0x0F18, 0x4A6F, 0x206D, 0xA03C, 0x4400, 0xE540, 0xFFE0, 0x57C0,
    0x4E56, 0xFFFE, 0x41FA, 0x3028, 0x2E1F, 0x2054, 0x0C40, 0x4EF9,
    0x7FFF, 0x0240, 0x1B7C, 0x206E, 0x544F, 0x4267, 0xA050, 0x4880,
    0x48E7, 0x6906, 0x0074, 0x57C1, 0x487A, 0xFFF0, 0xA05C, 0x2F2E,
    0x101F, 0x6704, 0x046A, 0xFFD6, 0x322E, 0x0A00, 0x0158, 0x0116,
    0x2005, 0x6006, 0x5C4F, 0xFFC8, 0x0004, 0x397C, 0x6B18, 0x0026,
    0x42A7, 0xFFCC, 0x3F06, 0x206B, 0x422C, 0x4ED0, 0x1800, 0x285F,
    0x4EAD, 0x5240, 0x286D, 0xA060, 0x0050, 0xFFD8, 0x0007, 0x43EE,
    0xFFCE, 0x302B, 0x0028, 0xF000, 0x41EC, 0x102D, 0x2F06, 0x197C,
};


int
lisa_unpackcode(uint8_t *packed, lisa_longint packed_size,
				uint8_t *unpacked, lisa_longint unpacked_size,
				lisa_PackTable * LISA_NULLABLE table)
{
    // Only support v1 tables.
    if (table && (table->packversion != 1)) return -1;

    // Only support even-sized buffers.
    if (packed_size % 2) return -1;
    if (unpacked_size % 2) return -1;

    const uint16_t *words = table ? table->words : lisa_unpackcode_table;

    // Work *backwards* through the buffers.

    int packed_idx = packed_size - 1;
    int unpacked_idx = unpacked_size - 1;

    // Handle the final byte, and possibly a slack byte.

    uint8_t final_byte = packed[packed_idx--];
    int max_bit;
    if (final_byte % 2) {
        // If the final byte is odd, there's no slack byte, and
        //    ((final_byte - 1) / 2)
        // is the index of the last bit to care about in the final
        // encoded flag byte.
        max_bit = ((final_byte - 1) / 2);
    } else {
        // If the final byte is even, there is a slack byte, and
        //     (final_byte / 2)
        // is the index of the last bit to care about in the final
        // encoded flag byte.
        max_bit = (final_byte / 2);
        packed_idx--; // skip slack byte
    }

    while (packed_idx > 0) {
        uint8_t flags = packed[packed_idx--];
        for (int i = 0; i <= max_bit; i++) {
            int flag = BIT(flags, i);

            if (flag) {
                // Copy from table
                uint8_t word_idx = packed[packed_idx--];
                uint16_t word = swapu16be(words[word_idx]);
                unpacked[unpacked_idx--] = (word & 0xff00) >> 8;
                unpacked[unpacked_idx--] = (word & 0x00ff);
            } else {
                // Copy directly from input
                unpacked[unpacked_idx--] = packed[packed_idx--];
                unpacked[unpacked_idx--] = packed[packed_idx--];
            }
        }

        // Only the first bit is allowed to be less than 7.
        if (max_bit < 7) {
            max_bit = 7;
        }
    }

    return 0;
}


LISA_SOURCE_END
