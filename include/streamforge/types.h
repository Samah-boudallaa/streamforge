#ifndef STREAMFORGE_TYPES_H
#define STREAMFORGE_TYPES_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SF_TYPE_INVALID = 0,

    SF_TYPE_BOOL,

    SF_TYPE_INT8,
    SF_TYPE_INT16,
    SF_TYPE_INT32,
    SF_TYPE_INT64,

    SF_TYPE_UINT8,
    SF_TYPE_UINT16,
    SF_TYPE_UINT32,
    SF_TYPE_UINT64,

    SF_TYPE_FLOAT32,
    SF_TYPE_FLOAT64,

    SF_TYPE_STRING,
    SF_TYPE_BINARY
} sf_type_t;

/* Returns the human-readable name of a data type. */
const char *sf_type_name(sf_type_t type);

/*
 * Returns the in-memory size of a fixed-width type.
 * Returns 0 for variable-width or invalid types.
 */
size_t sf_type_size(sf_type_t type);

/* Returns true when the type represents a numeric value. */
bool sf_type_is_numeric(sf_type_t type);

#ifdef __cplusplus
}
#endif

#endif
