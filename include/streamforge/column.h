#ifndef STREAMFORGE_COLUMN_H
#define STREAMFORGE_COLUMN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "streamforge/error.h"
#include "streamforge/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A typed, append-only column.
 *
 * Fixed-width types: `data` holds `length` contiguous values.
 * STRING: `offsets` holds length + 1 entries; value i occupies
 * data[offsets[i] .. offsets[i + 1]). Strings are NOT NUL-terminated.
 *
 * `validity` is a bitmap: bit i set means value i is non-null.
 * Null slots hold a zero value (or an empty range for strings).
 */
typedef struct {
    sf_type_t type;
    size_t length;
    size_t capacity;      /* capacity in values */
    size_t null_count;
    uint8_t *validity;    /* bitmap, 1 = valid */
    void *data;           /* values, or string bytes */
    uint64_t *offsets;    /* STRING only */
    size_t data_size;     /* STRING only: bytes used */
    size_t data_capacity; /* STRING only: bytes allocated */
} sf_column_t;

/* Initializes an empty column. No memory is allocated until the first append. */
sf_status_t sf_column_init(sf_column_t *column, sf_type_t type, sf_error_t *error);

/* Releases all memory owned by the column. Safe to call on a zeroed column. */
void sf_column_destroy(sf_column_t *column);

sf_status_t sf_column_append_null(sf_column_t *column, sf_error_t *error);
sf_status_t sf_column_append_int64(sf_column_t *column, int64_t value, sf_error_t *error);
sf_status_t sf_column_append_float64(sf_column_t *column, double value, sf_error_t *error);
sf_status_t sf_column_append_string(sf_column_t *column, const char *value, size_t length,
                                    sf_error_t *error);

/* Accessors. Precondition: index < column->length (and matching type). */
bool sf_column_is_valid(const sf_column_t *column, size_t index);
int64_t sf_column_get_int64(const sf_column_t *column, size_t index);
double sf_column_get_float64(const sf_column_t *column, size_t index);
const char *sf_column_get_string(const sf_column_t *column, size_t index, size_t *length);

#ifdef __cplusplus
}
#endif

#endif
