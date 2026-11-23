#include "streamforge/aggregate.h"

#include <string.h>

#define SF_FAIL(err, code, ...)                       \
    do {                                              \
        if ((err) != NULL) {                          \
            sf_error_set((err), (code), __VA_ARGS__); \
        }                                             \
        return (code);                                \
    } while (0)

/* Validates arguments and prepares *out as a "no value" result. */
static sf_status_t begin(const sf_column_t *c, sf_scalar_t *out, const char *fn,
                         sf_error_t *error) {
    if (c == NULL || out == NULL) {
        SF_FAIL(error, SF_STATUS_INVALID_ARGUMENT, "%s: NULL argument", fn);
    }
    memset(out, 0, sizeof *out);
    if (c->type != SF_TYPE_INT64 && c->type != SF_TYPE_FLOAT64) {
        SF_FAIL(error, SF_STATUS_TYPE_ERROR, "%s is not defined for %s columns", fn,
                sf_type_name(c->type));
    }
    out->type = c->type;
    out->is_null = true;
    return SF_STATUS_OK;
}

static bool has_values(const sf_column_t *c) {
    return c->length > c->null_count;
}

sf_status_t sf_agg_count(const sf_column_t *column, size_t *out) {
    if (column == NULL || out == NULL) {
        return SF_STATUS_INVALID_ARGUMENT;
    }
    *out = column->length - column->null_count;
    return SF_STATUS_OK;
}

sf_status_t sf_agg_sum(const sf_column_t *c, sf_scalar_t *out, sf_error_t *error) {
    sf_status_t st = begin(c, out, "SUM", error);
    if (st != SF_STATUS_OK || !has_values(c)) {
        return st;
    }

    const bool all_valid = (c->null_count == 0);

    if (c->type == SF_TYPE_INT64) {
        const int64_t *d = (const int64_t *)c->data;
        int64_t acc = 0;
        for (size_t i = 0; i < c->length; i++) {
            if (!all_valid && !sf_column_is_valid(c, i)) {
                continue;
            }
            int64_t v = d[i];
            if ((v > 0 && acc > INT64_MAX - v) || (v < 0 && acc < INT64_MIN - v)) {
                SF_FAIL(error, SF_STATUS_INVALID_ARGUMENT, "integer overflow in SUM at row %zu",
                        i + 1u);
            }
            acc += v;
        }
        out->as.i64 = acc;
    } else {
        const double *d = (const double *)c->data;
        double acc = 0.0;
        for (size_t i = 0; i < c->length; i++) {
            if (!all_valid && !sf_column_is_valid(c, i)) {
                continue;
            }
            acc += d[i];
        }
        out->as.f64 = acc;
    }
    out->is_null = false;
    return SF_STATUS_OK;
}

static sf_status_t extreme(const sf_column_t *c, bool want_max, const char *fn, sf_scalar_t *out,
                           sf_error_t *error) {
    sf_status_t st = begin(c, out, fn, error);
    if (st != SF_STATUS_OK || !has_values(c)) {
        return st;
    }

    const bool all_valid = (c->null_count == 0);
    bool seen = false;

    if (c->type == SF_TYPE_INT64) {
        const int64_t *d = (const int64_t *)c->data;
        int64_t best = 0;
        for (size_t i = 0; i < c->length; i++) {
            if (!all_valid && !sf_column_is_valid(c, i)) {
                continue;
            }
            if (!seen || (want_max ? d[i] > best : d[i] < best)) {
                best = d[i];
                seen = true;
            }
        }
        out->as.i64 = best;
    } else {
        const double *d = (const double *)c->data;
        double best = 0.0;
        for (size_t i = 0; i < c->length; i++) {
            if (!all_valid && !sf_column_is_valid(c, i)) {
                continue;
            }
            if (!seen || (want_max ? d[i] > best : d[i] < best)) {
                best = d[i];
                seen = true;
            }
        }
        out->as.f64 = best;
    }
    out->is_null = false;
    return SF_STATUS_OK;
}

sf_status_t sf_agg_min(const sf_column_t *c, sf_scalar_t *out, sf_error_t *error) {
    return extreme(c, false, "MIN", out, error);
}

sf_status_t sf_agg_max(const sf_column_t *c, sf_scalar_t *out, sf_error_t *error) {
    return extreme(c, true, "MAX", out, error);
}

sf_status_t sf_agg_avg(const sf_column_t *c, sf_scalar_t *out, sf_error_t *error) {
    sf_status_t st = begin(c, out, "AVG", error);
    if (st != SF_STATUS_OK || !has_values(c)) {
        out->type = SF_TYPE_FLOAT64;
        return st;
    }

    const bool all_valid = (c->null_count == 0);
    double acc = 0.0;

    for (size_t i = 0; i < c->length; i++) {
        if (!all_valid && !sf_column_is_valid(c, i)) {
            continue;
        }
        acc += (c->type == SF_TYPE_INT64) ? (double)((const int64_t *)c->data)[i]
                                          : ((const double *)c->data)[i];
    }

    out->type = SF_TYPE_FLOAT64;
    out->as.f64 = acc / (double)(c->length - c->null_count);
    out->is_null = false;
    return SF_STATUS_OK;
}
