//  array_utils.h
//  General-Purpose Utilities
//
//  Copyright © 2026 Christopher M. Hanson. All rights reserved.
//  See file COPYING for details.

#ifndef __ARRAY_UTILS__H__
#define __ARRAY_UTILS__H__

#include "utils_defines.h"

#include <stdlib.h>

UTILS_HEADER_BEGIN


/*! An extensible array of pointers. */
struct ptr_array;
typedef struct ptr_array ptr_array;


/*! Create a new pointer array with an initial capacity. */
UTILS_EXTERN
ptr_array * _Nullable
ptr_array_create(size_t capacity);

/*! Free a pointer array. */
UTILS_EXTERN
void
ptr_array_free(ptr_array * _Nullable array);

/*! Get the number of items in a pointer array. */
UTILS_EXTERN
size_t
ptr_array_count(ptr_array *array);

/*! Get the item at the given index in a pointer array. */
UTILS_EXTERN
void *
ptr_array_item_at_index(ptr_array *array, size_t idx);

/*! Append the item to a pointer array. */
UTILS_EXTERN
void
ptr_array_append(ptr_array *array, void *item);


UTILS_HEADER_END

#endif /* __BIT_UTILS__H__ */
