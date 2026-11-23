#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "streamforge/aggregate.h"
#include "streamforge/csv_loader.h"
#include "streamforge/error.h"
#include "streamforge/table.h"
#include "streamforge/types.h"

#define CELL_MAX 48u
#define DEFAULT_HEAD_ROWS 10u

typedef sf_status_t (*agg_fn)(const sf_column_t *, sf_scalar_t *, sf_error_t *);

static const struct {
    const char *name;
    agg_fn fn;
} AGGREGATES[] = {
    {"sum", sf_agg_sum},
    {"min", sf_agg_min},
    {"max", sf_agg_max},
    {"avg", sf_agg_avg},
};

static void usage(FILE *out) {
    fputs("StreamForge: an analytical data engine written in C17\n"
          "\n"
          "Usage:\n"
          "  streamforge schema <file.csv>\n"
          "  streamforge head   <file.csv> [rows]\n"
          "  streamforge agg    <file.csv> <column> <count|sum|min|max|avg>\n",
          out);
}

static bool parse_count(const char *s, size_t *out) {
    char *end = NULL;
    errno = 0;
    unsigned long v = strtoul(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || s[0] == '-') {
        return false;
    }
    *out = (size_t)v;
    return true;
}

static bool load(const char *path, sf_table_t *table) {
    sf_error_t err;
    sf_error_clear(&err);
    if (sf_csv_load_file(path, NULL, table, &err) != SF_STATUS_OK) {
        fprintf(stderr, "error: %s\n", err.message);
        return false;
    }
    return true;
}

static void format_cell(const sf_column_t *col, size_t row, char *buf, size_t cap) {
    if (!sf_column_is_valid(col, row)) {
        snprintf(buf, cap, "NULL");
        return;
    }
    switch (col->type) {
    case SF_TYPE_INT64:
        snprintf(buf, cap, "%" PRId64, sf_column_get_int64(col, row));
        break;
    case SF_TYPE_FLOAT64:
        snprintf(buf, cap, "%.10g", sf_column_get_float64(col, row));
        break;
    case SF_TYPE_STRING: {
        size_t len = 0;
        const char *s = sf_column_get_string(col, row, &len);
        size_t keep = len;
        bool truncated = false;
        if (keep > cap - 4u) {
            keep = cap - 4u;
            truncated = true;
        }
        for (size_t i = 0; i < keep; i++) {
            buf[i] = ((unsigned char)s[i] < 32u) ? ' ' : s[i];
        }
        if (truncated) {
            memcpy(buf + keep, "...", 3);
            keep += 3u;
        }
        buf[keep] = '\0';
        break;
    }
    default:
        snprintf(buf, cap, "?");
        break;
    }
}

static int cmd_schema(const char *path) {
    sf_table_t t;
    if (!load(path, &t)) {
        return 1;
    }

    size_t name_width = 4;
    for (size_t i = 0; i < t.schema.num_fields; i++) {
        size_t n = strlen(t.schema.fields[i].name);
        if (n > name_width) {
            name_width = n;
        }
    }

    printf("%s: %zu rows, %zu columns\n\n", path, t.num_rows, t.schema.num_fields);
    printf("  %-3s  %-*s  %-8s  %s\n", "#", (int)name_width, "name", "type", "nulls");
    for (size_t i = 0; i < t.schema.num_fields; i++) {
        printf("  %-3zu  %-*s  %-8s  %zu\n", i, (int)name_width, t.schema.fields[i].name,
               sf_type_name(t.schema.fields[i].type), t.columns[i].null_count);
    }

    sf_table_destroy(&t);
    return 0;
}

static int cmd_head(const char *path, size_t rows) {
    sf_table_t t;
    if (!load(path, &t)) {
        return 1;
    }

    const size_t ncols = t.schema.num_fields;
    const size_t shown = (rows < t.num_rows) ? rows : t.num_rows;
    char cell[CELL_MAX];

    size_t *width = malloc(ncols * sizeof *width);
    if (width == NULL) {
        fprintf(stderr, "error: out of memory\n");
        sf_table_destroy(&t);
        return 1;
    }

    for (size_t c = 0; c < ncols; c++) {
        width[c] = strlen(t.schema.fields[c].name);
        for (size_t r = 0; r < shown; r++) {
            format_cell(&t.columns[c], r, cell, sizeof cell);
            size_t n = strlen(cell);
            if (n > width[c]) {
                width[c] = n;
            }
        }
    }

    for (size_t c = 0; c < ncols; c++) {
        printf("%s%-*s", c == 0 ? "" : "  ", (int)width[c], t.schema.fields[c].name);
    }
    printf("\n");
    for (size_t c = 0; c < ncols; c++) {
        printf("%s", c == 0 ? "" : "  ");
        for (size_t k = 0; k < width[c]; k++) {
            putchar('-');
        }
    }
    printf("\n");

    for (size_t r = 0; r < shown; r++) {
        for (size_t c = 0; c < ncols; c++) {
            format_cell(&t.columns[c], r, cell, sizeof cell);
            printf("%s%-*s", c == 0 ? "" : "  ", (int)width[c], cell);
        }
        printf("\n");
    }
    printf("\n(showing %zu of %zu rows)\n", shown, t.num_rows);

    free(width);
    sf_table_destroy(&t);
    return 0;
}

static void print_scalar(const sf_scalar_t *r) {
    if (r->is_null) {
        printf("NULL\n");
    } else if (r->type == SF_TYPE_INT64) {
        printf("%" PRId64 "\n", r->as.i64);
    } else {
        printf("%.10g\n", r->as.f64);
    }
}

static int cmd_agg(const char *path, const char *column, const char *func) {
    sf_table_t t;
    if (!load(path, &t)) {
        return 1;
    }

    int rc = 1;
    size_t idx = 0;
    if (!sf_table_find_column(&t, column, &idx)) {
        fprintf(stderr, "error: no such column '%s'\n", column);
        goto done;
    }

    if (strcmp(func, "count") == 0) {
        size_t n = 0;
        (void)sf_agg_count(&t.columns[idx], &n);
        printf("count(%s) = %zu\n", column, n);
        rc = 0;
        goto done;
    }

    for (size_t i = 0; i < sizeof AGGREGATES / sizeof AGGREGATES[0]; i++) {
        if (strcmp(func, AGGREGATES[i].name) == 0) {
            sf_scalar_t r;
            sf_error_t err;
            sf_error_clear(&err);
            if (AGGREGATES[i].fn(&t.columns[idx], &r, &err) != SF_STATUS_OK) {
                fprintf(stderr, "error: %s\n", err.message);
                goto done;
            }
            printf("%s(%s) = ", func, column);
            print_scalar(&r);
            rc = 0;
            goto done;
        }
    }
    fprintf(stderr, "error: unknown function '%s' (use count, sum, min, max or avg)\n", func);

done:
    sf_table_destroy(&t);
    return rc;
}

int main(int argc, char **argv) {
    if (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        usage(stdout);
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "schema") == 0) {
        return cmd_schema(argv[2]);
    }
    if ((argc == 3 || argc == 4) && strcmp(argv[1], "head") == 0) {
        size_t rows = DEFAULT_HEAD_ROWS;
        if (argc == 4 && !parse_count(argv[3], &rows)) {
            fprintf(stderr, "error: '%s' is not a valid row count\n", argv[3]);
            return 2;
        }
        return cmd_head(argv[2], rows);
    }
    if (argc == 5 && strcmp(argv[1], "agg") == 0) {
        return cmd_agg(argv[2], argv[3], argv[4]);
    }

    usage(stderr);
    return 2;
}
