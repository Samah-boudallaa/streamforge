#ifndef STREAMFORGE_CSV_LOADER_H
#define STREAMFORGE_CSV_LOADER_H

#include <stdbool.h>
#include <stddef.h>

#include "streamforge/csv.h"
#include "streamforge/error.h"
#include "streamforge/table.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    sf_csv_dialect_t dialect;
    bool has_header;    /* first record holds column names (default: true) */
    size_t max_columns; /* upper bound on columns per record (default: 4096) */
} sf_csv_options_t;

sf_csv_options_t sf_csv_default_options(void);

/*
 * Loads CSV text into a typed columnar table.
 *
 * Column types are inferred from the whole input (INT64, FLOAT64, STRING).
 * Empty fields and the tokens NULL, null, NA, N/A become nulls.
 * Every record must have the same number of fields as the first one.
 *
 * `options` may be NULL for defaults. On failure `out` is left empty and
 * `error` (if not NULL) describes the problem, including the line number.
 */
sf_status_t sf_csv_load_buffer(const char *data, size_t length, const sf_csv_options_t *options,
                               sf_table_t *out, sf_error_t *error);

sf_status_t sf_csv_load_file(const char *path, const sf_csv_options_t *options, sf_table_t *out,
                             sf_error_t *error);

#ifdef __cplusplus
}
#endif

#endif
