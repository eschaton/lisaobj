//  utils_defines.h
//  General-Purpose Utilities
//
//  Copyright © 2026 Christopher M. Hanson. All rights reserved.
//  See file COPYING for details.

#ifndef __UTILS__DEFINES__H__
#define __UTILS__DEFINES__H__


#if defined(__cplusplus) || defined(cplusplus)
#define UTILS_EXTERN extern "C"
#else
#define UTILS_EXTERN extern
#endif


#define UTILS_PACKED	__attribute__((packed))


#define UTILS_NONNULL	_Nonnull
#define UTILS_NULLABLE	_Nullable

#define UTILS_ALLOW_NULLABILITY_EXTENSION	_Pragma("clang diagnostic ignored \"-Wnullability-extension\"")
#define UTILS_ALLOW_FIXED_ENUM_EXTENSION	_Pragma("clang diagnostic ignored \"-Wfixed-enum-extension\"")

#define UTILS_ASSUME_NONNULL_BEGIN _Pragma("clang assume_nonnull begin")
#define UTILS_ASSUME_NONNULL_END   _Pragma("clang assume_nonnull end")

#define UTILS_HEADER_BEGIN		UTILS_ASSUME_NONNULL_BEGIN \
                                UTILS_ALLOW_NULLABILITY_EXTENSION \
                                UTILS_ALLOW_FIXED_ENUM_EXTENSION
#define UTILS_HEADER_END		UTILS_ASSUME_NONNULL_END

#define UTILS_SOURCE_BEGIN		UTILS_ASSUME_NONNULL_BEGIN \
                                UTILS_ALLOW_NULLABILITY_EXTENSION \
                                UTILS_ALLOW_FIXED_ENUM_EXTENSION
#define UTILS_SOURCE_END		UTILS_ASSUME_NONNULL_END


#endif /* __UTILS__DEFINES__H__ */
