//  lisa_objio.h
//  Part of LisaUtils.
//
//  Copyright © 2026 Christopher M. Hanson. All rights reserved.
//  See file COPYING for details.

#ifndef __LISA__OBJIO__H__
#define __LISA__OBJIO__H__

#include "lisa_defines.h"
#include "lisa_types.h"

LISA_HEADER_BEGIN


/*! Types of blocks in a Lisa executable/object file. */
enum lisa_obj_block_type: uint8_t {
    ModuleName			= 0x80,
    EndBlock			= 0x81,
    EntryPoint			= 0x82,
    External			= 0x83,
    StartAddress		= 0x84,
    CodeBlock			= 0x85,
    Relocation			= 0x86,
    CommonRelocation	= 0x87,
    ShortExternal		= 0x89,
    OldExecutable		= 0x8F,
    UnitBlock			= 0x92,
    PhysicalExec		= 0x97,
    Executable			= 0x98,
    VersionCtrl			= 0x99,
    SegmentTable		= 0x9A,
    UnitTable			= 0x9B,
    SegLocation			= 0x9C,
    UnitLocation		= 0x9D,
    StringBlock			= 0x9E,
    PackedCode			= 0xA0,
    PackTable			= 0xA1,
    OSData				= 0xB2,
    EOFMark				= 0x00,
};
typedef enum lisa_obj_block_type lisa_obj_block_type;


/*! Get a string corresponding to the given block type. */
LISA_EXTERN
const char *
lisa_obj_block_type_string(lisa_obj_block_type t);


/*! A module name block. ($80) */
struct lisa_ModuleName {
    lisa_ObjName		ModuleName;
    lisa_ObjName		SegmentName;
    lisa_longint		CSize;
} LISA_PACKED;
typedef struct lisa_ModuleName lisa_ModuleName;

/*! An end block. ($81) */
struct lisa_EndBlock {
    lisa_longint		CSize;
} LISA_PACKED;
typedef struct lisa_EndBlock lisa_EndBlock;

/*! An entry point. ($82) */
struct lisa_EntryPoint {
    lisa_ObjName		LinkName;
    lisa_ObjName		UserName;
    lisa_SegAddr		Loc;
} LISA_PACKED;
typedef struct lisa_EntryPoint lisa_EntryPoint;

/*! An external reference block. ($83) */
struct lisa_External {
    lisa_ObjName        LinkName;
    lisa_ObjName        UserName;
    lisa_SegAddr		Ref[1];		//!< count derived from block size
} LISA_PACKED;
typedef struct lisa_External lisa_External;

/*! A start address block. ($84) */
struct lisa_StartAddress {
    lisa_SegAddr		Start;	//!< Starting address relative to this block
    lisa_longint		GSize;	//!< Number of bytes in global data area.
} LISA_PACKED;
typedef struct lisa_StartAddress lisa_StartAddress;

/*! A raw object code block. ($85) */
struct lisa_CodeBlock {
    lisa_SegAddr		Addr;
    uint8_t				code[2];	//!< actually unpacked code bytes
} LISA_PACKED;
typedef struct lisa_CodeBlock lisa_CodeBlock;

/*! An old-style Lisa relocation block. ($86) */
struct lisa_Relocation {
    lisa_SegAddr		Ref[1];		//!< count derived from block size
} LISA_PACKED;
typedef struct lisa_Relocation lisa_Relocation;

/*! An old-style Lisa common relocation block. ($87) */
struct lisa_CommonRelocation {
    lisa_ObjName		CommonName;
    lisa_SegAddr		Ref[1];		//!< count derived from block size
} LISA_PACKED;
typedef struct lisa_CommonRelocation lisa_CommonRelocation;

/*! A Lisa short external. ($89) */
struct lisa_ShortExternal {
    lisa_ObjName		LinkName;
    lisa_ObjName		UserName;
    lisa_integer		ShortRef[1];	//!< count derived from block size
} LISA_PACKED;
typedef struct lisa_ShortExternal lisa_ShortExternal;

/*! A Lisa unit type. */
enum lisa_UnitType: lisa_integer {
    RegularUnit = 0,
    IntrinsicUnit = 1,
    SharedUnit = 2,
};
typedef enum lisa_UnitType lisa_UnitType;

/*! Get a string corresponding to the given unit type. */
LISA_EXTERN
const char *
lisa_UnitType_string(lisa_UnitType t);

/*! A unit block. ($92) */
struct lisa_UnitBlock {
    lisa_ObjName		UnitName;
    lisa_FileAddr		CodeAddr;
    lisa_FileAddr		TextAddr;
    lisa_longint		TextSize;
    lisa_longint		GlobalSize;
    lisa_UnitType		UnitType;
}LISA_PACKED;
typedef struct lisa_UnitBlock lisa_UnitBlock;

/*! A Lisa jump table segment variant. */
struct lisa_JTSegVariant {
    lisa_FileAddr		SegmentAddr;
    lisa_integer		SizePacked;
    lisa_integer		SizeUnpacked;
    lisa_MemAddr		MemLoc;
} LISA_PACKED;
typedef struct lisa_JTSegVariant lisa_JTSegVariant;

/*! A Lisa jump table segment table. */
struct lisa_JTSegVariantTable {
    lisa_integer		numSegs;
    lisa_JTSegVariant	variants[1];
} LISA_PACKED;
typedef struct lisa_JTSegVariantTable lisa_JTSegVariantTable;

/*! A Lisa jump table variant. */
struct lisa_JTVariant {
    lisa_integer		JumpL;
    lisa_MemAddr		AbsAddr;
} LISA_PACKED;
typedef struct lisa_JTVariant lisa_JTVariant;

/*! A Lisa jump table descriptor table. */
struct lisa_JTVariantTable {
    lisa_integer		numDescriptors;
    lisa_JTVariant		variants[1];
} LISA_PACKED;
typedef struct lisa_JTVariantTable lisa_JTVariantTable;

/*! A Lisa executable info block. ($98) */
struct lisa_Executable {
    lisa_MemAddr		JTLaddr;
    lisa_longint		JTSize;
    lisa_longint		DataSize;
    lisa_longint		MainSize;
    lisa_longint		JTSegDelta;
    lisa_longint		StkSegDelta;
    lisa_longint		DynStack;
    lisa_longint		MaxStack;
    lisa_longint		MinHeap;
    lisa_longint		MaxHeap;
} LISA_PACKED;
typedef struct lisa_Executable lisa_Executable;

/*! Get a pointer to an executable's JTSegVariantTable. */
LISA_EXTERN
lisa_JTSegVariantTable *
lisa_Executable_JTSegVariantTable(lisa_Executable *executable);

/*! Get a pointer to an executable's JTVariantTable. */
LISA_EXTERN
lisa_JTVariantTable *
lisa_Executable_JTVariantTable(lisa_Executable *executable);

/*! A Lisa executable version control block. ($99) */
struct lisa_VersionCtrl {
    lisa_longint		sysNum;
    lisa_longint		minSys;
    lisa_longint		maxSys;
    lisa_longint		Reserv1;
    lisa_longint		Reserv2;
    lisa_longint		Reserv3;
} LISA_PACKED;
typedef struct lisa_VersionCtrl lisa_VersionCtrl;

/*! A Lisa segment table variant item. */
struct lisa_SegVariant {
    // 1st release only
    lisa_ObjName		SegName;
    lisa_integer		SegNumber;
    lisa_longint		Version1;
    lisa_longint		Version2;
} LISA_PACKED;
typedef struct lisa_SegVariant lisa_SegVariant;

/*! A Lisa segment table block. ($9A) */
struct lisa_SegmentTable {
    lisa_integer		nSegments;
    lisa_SegVariant	variants[1];
} LISA_PACKED;
typedef struct lisa_SegmentTable lisa_SegmentTable;

/*! A Lisa unit table variant item. */
struct lisa_UnitVariant {
    // 1st release only
    lisa_ObjName		UnitName;
    lisa_integer		UnitNumber;
    lisa_UnitType		UnitType;
} LISA_PACKED;
typedef struct lisa_UnitVariant lisa_UnitVariant;

/*! A Lisa unit table block. ($9B) */
struct lisa_UnitTable {
    lisa_integer		nUnits;
    lisa_integer		maxunit;
    lisa_UnitVariant	variants[1];
} LISA_PACKED;
typedef struct lisa_UnitTable lisa_UnitTable;

/*! A Lisa segment location block variant. */
struct lisa_SegLocVariant {
    lisa_ObjName		SegName;
    lisa_integer		SegNumber;
    lisa_longint		Version1;
    lisa_longint		Version2;
    lisa_integer		FileNumber;
    lisa_FileAddr		FileLocation;
    lisa_integer		SizePacked;
    lisa_integer		SizeUnpacked;
} LISA_PACKED;
typedef struct lisa_SegLocVariant lisa_SegLocVariant;

/*! A Lisa segment location block. ($9C) */
struct lisa_SegLocation {
    lisa_integer		nSegments;
    lisa_SegLocVariant	variants[1];
} LISA_PACKED;
typedef struct lisa_SegLocation lisa_SegLocation;

/*! A Lisa unit location variant item. */
struct lisa_UnitLVariant {
    lisa_ObjName		UnitName;
    lisa_integer		UnitNumber;
    uint8_t				FileNumber;
    uint8_t				UnitType;
    lisa_longint		DataSize;
} LISA_PACKED;
typedef struct lisa_UnitLVariant lisa_UnitLVariant;

/*! A Lisa unit location block. ($9D) */
struct lisa_UnitLocation {
    lisa_integer		nUnits;
    lisa_UnitLVariant	variants[1];
} LISA_PACKED;
typedef struct lisa_UnitLocation lisa_UnitLocation;

/*! A Lisa string block variant. */
struct lisa_StringVariant {
    lisa_integer		FileNumber;	//!< Index
    lisa_FileAddr		NameAddr;	//!< File address of name string
} LISA_PACKED;
typedef struct lisa_StringVariant lisa_StringVariant;

/*! A Lisa string block. ($9E) */
struct lisa_StringBlock {
    lisa_integer		nStrings;
    lisa_StringVariant	variants[1];
} LISA_PACKED;
typedef struct lisa_StringBlock lisa_StringBlock;

/*! A Lisa executable packed code block. ($A0) */
struct lisa_PackedCode {
    lisa_MemAddr		addr;		//!< load address of code
    lisa_longint		csize;		//!< unpacked size of code
    uint8_t				code[2];	//!< actually packed code bytes
} LISA_PACKED;
typedef struct lisa_PackedCode lisa_PackedCode;

/*! A Lisa executable packing table. ($A1) */
struct lisa_PackTable {
    lisa_longint		packversion;	//!< only ever 1
    uint16_t			words[256];		//!< always 256 words for v1
} LISA_PACKED;
typedef struct lisa_PackTable lisa_PackTable;

/*! A Lisa executable's OS data. ($B2) */
struct lisa_OSData {
    uint8_t				bitmap[16];		//!< bitmap of segments to preload
} LISA_PACKED;
typedef struct lisa_OSData lisa_OSData;

/*! A Lisa executable/object file. (Opaque!) */
struct lisa_objfile;
typedef struct lisa_objfile lisa_objfile;


/*! A block within a Lisa executable/object file. (Opaque!) */
struct lisa_obj_block;
typedef struct lisa_obj_block lisa_obj_block;


/*! Open the given Lisa executable/object file for reading. */
LISA_EXTERN
lisa_objfile * _Nullable
lisa_objfile_open(const char * _Nonnull path);

/*! Close the given Lisa executable/object file. */
LISA_EXTERN
void
lisa_objfile_close(lisa_objfile * _Nullable ef);

/*! Get the count of blocks in the object file. */
LISA_EXTERN
lisa_integer
lisa_objfile_block_count(lisa_objfile *ef);

/*! Get the block at the given index. */
LISA_EXTERN
lisa_obj_block *
lisa_objfile_block_at_index(lisa_objfile *ef, lisa_integer idx);

/*! A pointer to some data at the given offset within the file. */
LISA_EXTERN
void *
lisa_objfile_data_at_offset(lisa_objfile *of,
                            lisa_FileAddr file_offset);

/*!
	Gets the Pascal string at \a offset within object file \a of, into
	\a cstr as a C string.

	- WARNING: `cstr` must be large enough to accommodate the string;
			   the safest thing is to just use a 256-byte buffer.
 */
LISA_EXTERN
void
lisa_objfile_copy_pstring_at_offset(lisa_objfile *of,
                                    char *cstr,
                                    lisa_FileAddr offset);

/*! Dump the contents of a block to `stdout`. */
LISA_EXTERN
void
lisa_obj_block_dump(lisa_obj_block *block);

/*!
 Unpack a buffer of packed code using a table. Passing
 NULL for the table uses the default Lisa OS table.
 */
LISA_EXTERN
int
lisa_unpackcode(uint8_t *packed, lisa_longint packed_size,
                uint8_t *unpacked, lisa_longint unpacked_size,
                lisa_PackTable * _Nullable table);


LISA_HEADER_END

#endif /* __LISA__OBJIO__H__ */
