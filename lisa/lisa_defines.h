//  lisa_defines.h
//  Part of LisaUtils.
//
//  Copyright © 2026 Christopher M. Hanson. All rights reserved.
//  See file COPYING for details.

#ifndef __LISA__DEFINES__H__
#define __LISA__DEFINES__H__

#include <stdbool.h>
#include <stdint.h>


#if defined(__cplusplus) || defined(cplusplus)
#define LISA_EXTERN extern "C"
#else
#define LISA_EXTERN extern
#endif


#define LISA_PACKED __attribute__((packed))
#define LISA_NONNULL _Nonnull
#define LISA_NULLABLE _Nullable

#define LISA_ALLOW_NULLABILITY_EXTENSION	_Pragma("clang diagnostic ignored \"-Wnullability-extension\"")
#define LISA_ALLOW_FIXED_ENUM_EXTENSION		_Pragma("clang diagnostic ignored \"-Wfixed-enum-extension\"")

#define LISA_ASSUME_NONNULL_BEGIN _Pragma("clang assume_nonnull begin")
#define LISA_ASSUME_NONNULL_END   _Pragma("clang assume_nonnull end")

#define LISA_HEADER_BEGIN	LISA_ASSUME_NONNULL_BEGIN \
                            LISA_ALLOW_NULLABILITY_EXTENSION \
                            LISA_ALLOW_FIXED_ENUM_EXTENSION
#define LISA_HEADER_END		LISA_ASSUME_NONNULL_END

#define LISA_SOURCE_BEGIN	LISA_ASSUME_NONNULL_BEGIN \
                            LISA_ALLOW_NULLABILITY_EXTENSION \
                            LISA_ALLOW_FIXED_ENUM_EXTENSION

#define LISA_SOURCE_END		LISA_ASSUME_NONNULL_END

#endif /* __LISA__DEFINES__H__ */
