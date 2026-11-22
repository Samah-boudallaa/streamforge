#ifndef STREAMFORGE_CSV_H
#define STREAMFORGE_CSV_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    SF_CSV_OK = 0,
    SF_CSV_EOF,
    SF_CSV_ERR_UNTERMINATED_QUOTE,
    SF_CSV_ERR_BAD_QUOTE,
    SF_CSV_ERR_TOO_MANY_FIELDS,
    SF_CSV_ERR_SCRATCH_FULL
} sf_csv_status_t;

typedef struct {
    char delimiter; /* default ',' */
    char quote;     /* default '"' */
} sf_csv_dialect_t;

typedef struct {
    const char *data;
    size_t len;
    size_t pos;
    size_t line; /* 1-based line number of the next record */
    size_t record_line; /* 1-based line where the last record started */
    sf_csv_dialect_t dialect;
} sf_csv_reader_t;

/* A field is NUL-terminated and points into the caller's scratch buffer. */
typedef struct {
    const char *text;
    size_t len;
} sf_csv_field_t;

sf_csv_dialect_t sf_csv_default_dialect(void);

void sf_csv_reader_init(sf_csv_reader_t *r, const char *data, size_t len,
                        sf_csv_dialect_t dialect);

/*
 * Reads the next record. Blank lines are skipped.
 * Returns SF_CSV_OK, SF_CSV_EOF, or an error status.
 * Fields stay valid until the next call that reuses `scratch`.
 */
sf_csv_status_t sf_csv_next_record(sf_csv_reader_t *r, char *scratch, size_t scratch_cap,
                                   sf_csv_field_t *fields, size_t max_fields,
                                   size_t *out_count);

const char *sf_csv_status_string(sf_csv_status_t s);

#endif /* STREAMFORGE_CSV_H */
