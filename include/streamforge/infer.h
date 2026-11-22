#ifndef STREAMFORGE_INFER_H
#define STREAMFORGE_INFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "streamforge/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * All text arguments must be NUL-terminated at text[length]
 * (fields returned by the CSV tokenizer always are).
 */

/* Empty, "NULL", "null", "NA" and "N/A" are treated as null. */
bool sf_is_null_token(const char *text, size_t length);

/* Strict base-10 integer: optional sign, digits only, no overflow. */
bool sf_parse_int64(const char *text, size_t length, int64_t *out);

/* Decimal floating point only (no inf, nan, hex, or surrounding spaces). */
bool sf_parse_float64(const char *text, size_t length, double *out);

/* Narrowest type for a non-null value: INT64, FLOAT64 or STRING. */
sf_type_t sf_infer_value_type(const char *text, size_t length);

/* Combines two observations: INVALID < INT64 < FLOAT64 < STRING. */
sf_type_t sf_type_widen(sf_type_t current, sf_type_t observed);

#ifdef __cplusplus
}
#endif

#endif
