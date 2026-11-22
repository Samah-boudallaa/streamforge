#include "streamforge/csv_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "streamforge/column.h"
#include "streamforge/infer.h"

#define SF_CSV_INITIAL_SCRATCH 4096u
#define SF_CSV_DEFAULT_MAX_COLUMNS 4096u
#define SF_CSV_HARD_MAX_COLUMNS 1048576u

#define SF_SET_ERROR(err, code, ...)                  \
    do {                                              \
        if ((err) != NULL) {                          \
            sf_error_set((err), (code), __VA_ARGS__); \
        }                                             \
    } while (0)

typedef struct {
    char *scratch;
    size_t scratch_cap;
    sf_csv_field_t *fields;
    size_t max_fields;
} record_buffer_t;

sf_csv_options_t sf_csv_default_options(void) {
    sf_csv_options_t options;
    options.dialect = sf_csv_default_dialect();
    options.has_header = true;
    options.max_columns = SF_CSV_DEFAULT_MAX_COLUMNS;
    return options;
}

static char *dup_n(const char *s, size_t n) {
    char *p = malloc(n + 1u);
    if (p == NULL) {
        return NULL;
    }
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

/* Reads one record, growing the scratch buffer when needed. */
static sf_csv_status_t read_record(sf_csv_reader_t *reader, record_buffer_t *rb, size_t *count) {
    for (;;) {
        sf_csv_reader_t saved = *reader;
        sf_csv_status_t st = sf_csv_next_record(reader, rb->scratch, rb->scratch_cap, rb->fields,
                                                rb->max_fields, count);
        if (st != SF_CSV_ERR_SCRATCH_FULL) {
            return st;
        }

        *reader = saved;
        size_t new_cap = rb->scratch_cap * 2u;
        if (new_cap < rb->scratch_cap) {
            return SF_CSV_ERR_SCRATCH_FULL;
        }
        char *grown = realloc(rb->scratch, new_cap);
        if (grown == NULL) {
            return SF_CSV_ERR_SCRATCH_FULL; /* reported as out of memory by the caller */
        }
        rb->scratch = grown;
        rb->scratch_cap = new_cap;
    }
}

static sf_status_t report_csv_failure(sf_csv_status_t st, size_t line, size_t max_columns,
                                      sf_error_t *error) {
    switch (st) {
    case SF_CSV_ERR_SCRATCH_FULL:
        SF_SET_ERROR(error, SF_STATUS_OUT_OF_MEMORY, "line %zu: out of memory reading record",
                     line);
        return SF_STATUS_OUT_OF_MEMORY;
    case SF_CSV_ERR_TOO_MANY_FIELDS:
        SF_SET_ERROR(error, SF_STATUS_PARSE_ERROR, "line %zu: more than %zu columns", line,
                     max_columns);
        return SF_STATUS_PARSE_ERROR;
    default:
        SF_SET_ERROR(error, SF_STATUS_PARSE_ERROR, "line %zu: %s", line,
                     sf_csv_status_string(st));
        return SF_STATUS_PARSE_ERROR;
    }
}

static sf_status_t append_value(sf_column_t *column, const sf_csv_field_t *field, size_t line,
                                sf_error_t *error) {
    if (sf_is_null_token(field->text, field->len)) {
        return sf_column_append_null(column, error);
    }

    switch (column->type) {
    case SF_TYPE_INT64: {
        int64_t value = 0;
        if (!sf_parse_int64(field->text, field->len, &value)) {
            SF_SET_ERROR(error, SF_STATUS_INTERNAL_ERROR,
                         "line %zu: value '%s' does not match inferred type", line, field->text);
            return SF_STATUS_INTERNAL_ERROR;
        }
        return sf_column_append_int64(column, value, error);
    }
    case SF_TYPE_FLOAT64: {
        double value = 0.0;
        if (!sf_parse_float64(field->text, field->len, &value)) {
            SF_SET_ERROR(error, SF_STATUS_INTERNAL_ERROR,
                         "line %zu: value '%s' does not match inferred type", line, field->text);
            return SF_STATUS_INTERNAL_ERROR;
        }
        return sf_column_append_float64(column, value, error);
    }
    case SF_TYPE_STRING:
        return sf_column_append_string(column, field->text, field->len, error);
    default:
        SF_SET_ERROR(error, SF_STATUS_INTERNAL_ERROR, "line %zu: unsupported column type", line);
        return SF_STATUS_INTERNAL_ERROR;
    }
}

sf_status_t sf_csv_load_buffer(const char *data, size_t length, const sf_csv_options_t *options,
                               sf_table_t *out, sf_error_t *error) {
    if (out == NULL || (data == NULL && length > 0)) {
        SF_SET_ERROR(error, SF_STATUS_INVALID_ARGUMENT, "invalid arguments to sf_csv_load_buffer");
        return SF_STATUS_INVALID_ARGUMENT;
    }
    memset(out, 0, sizeof *out);

    sf_csv_options_t opts = (options != NULL) ? *options : sf_csv_default_options();
    if (opts.max_columns == 0) {
        opts.max_columns = SF_CSV_DEFAULT_MAX_COLUMNS;
    }
    if (opts.max_columns > SF_CSV_HARD_MAX_COLUMNS) {
        opts.max_columns = SF_CSV_HARD_MAX_COLUMNS;
    }

    sf_status_t status = SF_STATUS_OK;
    sf_table_t table;
    record_buffer_t rb;
    sf_type_t *types = NULL;
    sf_csv_reader_t reader;
    sf_csv_reader_t data_start;
    sf_csv_reader_t pass;
    sf_csv_status_t cst;
    size_t ncols = 0;
    size_t rows = 0;
    size_t n = 0;
    size_t i = 0;

    memset(&table, 0, sizeof table);
    memset(&rb, 0, sizeof rb);

    rb.scratch_cap = SF_CSV_INITIAL_SCRATCH;
    rb.scratch = malloc(rb.scratch_cap);
    rb.max_fields = opts.max_columns;
    rb.fields = malloc(rb.max_fields * sizeof *rb.fields);
    if (rb.scratch == NULL || rb.fields == NULL) {
        SF_SET_ERROR(error, SF_STATUS_OUT_OF_MEMORY, "out of memory");
        status = SF_STATUS_OUT_OF_MEMORY;
        goto done;
    }

    /* First record: header names, or just the column count. */
    sf_csv_reader_init(&reader, data, length, opts.dialect);
    cst = read_record(&reader, &rb, &n);
    if (cst == SF_CSV_EOF) {
        SF_SET_ERROR(error, SF_STATUS_PARSE_ERROR, "input contains no records");
        status = SF_STATUS_PARSE_ERROR;
        goto done;
    }
    if (cst != SF_CSV_OK) {
        status = report_csv_failure(cst, reader.record_line, opts.max_columns, error);
        goto done;
    }

    ncols = n;
    table.schema.fields = calloc(ncols, sizeof *table.schema.fields);
    table.columns = calloc(ncols, sizeof *table.columns);
    types = calloc(ncols, sizeof *types);
    if (table.schema.fields == NULL || table.columns == NULL || types == NULL) {
        SF_SET_ERROR(error, SF_STATUS_OUT_OF_MEMORY, "out of memory");
        status = SF_STATUS_OUT_OF_MEMORY;
        goto done;
    }
    table.schema.num_fields = ncols;

    for (i = 0; i < ncols; i++) {
        char *name = NULL;
        if (opts.has_header && rb.fields[i].len > 0) {
            name = dup_n(rb.fields[i].text, rb.fields[i].len);
        } else {
            char generated[32];
            snprintf(generated, sizeof generated, "column_%zu", i + 1u);
            name = dup_n(generated, strlen(generated));
        }
        if (name == NULL) {
            SF_SET_ERROR(error, SF_STATUS_OUT_OF_MEMORY, "out of memory");
            status = SF_STATUS_OUT_OF_MEMORY;
            goto done;
        }
        table.schema.fields[i].name = name;
    }

    if (!opts.has_header) {
        sf_csv_reader_init(&reader, data, length, opts.dialect); /* first record is data */
    }
    data_start = reader;

    /* Pass 1: infer one type per column. */
    pass = data_start;
    for (;;) {
        cst = read_record(&pass, &rb, &n);
        if (cst == SF_CSV_EOF) {
            break;
        }
        if (cst != SF_CSV_OK) {
            status = report_csv_failure(cst, pass.record_line, opts.max_columns, error);
            goto done;
        }
        if (n != ncols) {
            SF_SET_ERROR(error, SF_STATUS_PARSE_ERROR, "line %zu: expected %zu fields, found %zu",
                         pass.record_line, ncols, n);
            status = SF_STATUS_PARSE_ERROR;
            goto done;
        }
        for (i = 0; i < ncols; i++) {
            if (sf_is_null_token(rb.fields[i].text, rb.fields[i].len)) {
                continue;
            }
            types[i] = sf_type_widen(types[i],
                                     sf_infer_value_type(rb.fields[i].text, rb.fields[i].len));
        }
        rows++;
    }

    for (i = 0; i < ncols; i++) {
        if (types[i] == SF_TYPE_INVALID) {
            types[i] = SF_TYPE_STRING; /* column made only of nulls */
        }
        table.schema.fields[i].type = types[i];
        status = sf_column_init(&table.columns[i], types[i], error);
        if (status != SF_STATUS_OK) {
            goto done;
        }
    }

    /* Pass 2: fill the columns. */
    pass = data_start;
    for (;;) {
        cst = read_record(&pass, &rb, &n);
        if (cst == SF_CSV_EOF) {
            break;
        }
        if (cst != SF_CSV_OK) {
            status = report_csv_failure(cst, pass.record_line, opts.max_columns, error);
            goto done;
        }
        if (n != ncols) {
            SF_SET_ERROR(error, SF_STATUS_INTERNAL_ERROR, "line %zu: record changed between passes",
                         pass.record_line);
            status = SF_STATUS_INTERNAL_ERROR;
            goto done;
        }
        for (i = 0; i < ncols; i++) {
            status = append_value(&table.columns[i], &rb.fields[i], pass.record_line, error);
            if (status != SF_STATUS_OK) {
                goto done;
            }
        }
    }

    table.num_rows = rows;
    *out = table;
    memset(&table, 0, sizeof table);

done:
    free(types);
    free(rb.scratch);
    free(rb.fields);
    sf_table_destroy(&table);
    return status;
}

sf_status_t sf_csv_load_file(const char *path, const sf_csv_options_t *options, sf_table_t *out,
                             sf_error_t *error) {
    if (path == NULL || out == NULL) {
        SF_SET_ERROR(error, SF_STATUS_INVALID_ARGUMENT, "invalid arguments to sf_csv_load_file");
        return SF_STATUS_INVALID_ARGUMENT;
    }
    memset(out, 0, sizeof *out);

    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        SF_SET_ERROR(error, SF_STATUS_IO_ERROR, "cannot open '%s'", path);
        return SF_STATUS_IO_ERROR;
    }

    long size = -1;
    if (fseek(fp, 0, SEEK_END) == 0) {
        size = ftell(fp);
    }
    if (size < 0 || fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        SF_SET_ERROR(error, SF_STATUS_IO_ERROR, "cannot determine size of '%s'", path);
        return SF_STATUS_IO_ERROR;
    }

    char *buffer = malloc((size_t)size + 1u);
    if (buffer == NULL) {
        fclose(fp);
        SF_SET_ERROR(error, SF_STATUS_OUT_OF_MEMORY, "cannot allocate %ld bytes for '%s'", size,
                     path);
        return SF_STATUS_OUT_OF_MEMORY;
    }

    size_t got = fread(buffer, 1u, (size_t)size, fp);
    fclose(fp);
    if (got != (size_t)size) {
        free(buffer);
        SF_SET_ERROR(error, SF_STATUS_IO_ERROR, "failed to read '%s'", path);
        return SF_STATUS_IO_ERROR;
    }

    sf_status_t status = sf_csv_load_buffer(buffer, got, options, out, error);
    free(buffer);
    return status;
}
