//  array_utils.c
//  General-Purpose Utilities
//
//  Copyright © 2026 Christopher M. Hanson. All rights reserved.
//  See file COPYING for details.

#include "array_utils.h"

#include <assert.h>
#include <stdlib.h>

UTILS_SOURCE_BEGIN


struct ptr_array {
    void **storage;
    size_t count;
    size_t capacity;
};


ptr_array * _Nullable
ptr_array_create(size_t capacity)
{
    const size_t real_capacity = capacity > 0 ? capacity : 8;

    ptr_array *array = calloc(sizeof(ptr_array), 1);
    if (array == NULL) goto error;

    array->storage = calloc(sizeof(void *), real_capacity);
    if (array->storage == NULL) goto error;

    array->count = 0;
    array->capacity = real_capacity;

    return array;

error:
    ptr_array_free(array);
    return NULL;
}


void
ptr_array_free(ptr_array * _Nullable array)
{
    if (array) {
        free(array->storage);
        free(array);
    }
}


size_t
ptr_array_count(ptr_array *array)
{
    return array->count;
}


void *
ptr_array_item_at_index(ptr_array *array, size_t idx)
{
    assert(idx < array->count);

    return array->storage[idx];
}


void
ptr_array_grow_if_needed(ptr_array *array)
{
    if (array->count == array->capacity) {
        const size_t new_capacity = array->capacity + 8;
        array->storage = realloc(array->storage, sizeof(void *) * new_capacity);
        assert(array->storage != NULL);
        array->capacity = new_capacity;
    }
}


void
ptr_array_append(ptr_array *array, void *item)
{
    ptr_array_grow_if_needed(array);

    array->storage[array->count] = item;
    array->count += 1;
}


UTILS_SOURCE_END
