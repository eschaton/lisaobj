//  lisa_types.h
//  Part of LisaUtils.
//
//  Copyright © 2026 Christopher M. Hanson. All rights reserved.
//  See file COPYING for details.

#ifndef __LISA_TYPES__H__
#define __LISA_TYPES__H__

#include "lisa_defines.h"

LISA_HEADER_BEGIN


/*! A Lisa/Pascal INTEGER. */
typedef int16_t lisa_integer;

/*! A Lisa/Pascal LONGINT. */
typedef int32_t lisa_longint;

/*! A Lisa executable format object name, a short string. */
typedef char lisa_ObjName[8];

/*! A Lisa executable memory address. */
typedef lisa_longint lisa_MemAddr;

/*! A Lisa segment address. */
typedef lisa_longint lisa_SegAddr;

/*! A Lisa file address. */
typedef lisa_longint lisa_FileAddr;


LISA_HEADER_END

#endif /* __LISA_TYPES__H__ */
