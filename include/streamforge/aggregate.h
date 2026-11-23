#ifndef STREAMFORGE_AGGREGATE_H
#define STREAMFORGE_AGGREGATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "streamforge/column.h"
#include "streamforge/error.h"
#include "streamforge/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Result of an aggregation. is_null is true when no non-null input existed. */
typedef struct {
    sf_type_t type; /* INT64 or FLOAT64 */
    bool is_null;
    union {
        int64_t i64;
        double f64;
    } as;
} sf_scalar_t;

/* Number of non-null values. Works on every column type. */
sf_status_t sf_agg_count(const sf_column_t *column, size_t *out);

/*
 * Numeric aggregations (INT64 and FLOAT64 columns). Nulls are ignored.
 * STRING columns return SF_STATUS_TYPE_ERROR.
 *
 * SUM and MIN/MAX keep the column type. SUM on INT64 fails with
 * SF_STATUS_INVALID_ARGUMENT on overflow. AVG always returns FLOAT64
 * (INT64 inputs are accumulated as double).
 */
sf_status_t sf_agg_sum(const sf_column_t *column, sf_scalar_t *out, sf_error_t *error);
sf_status_t sf_agg_min(const sf_column_t *column, sf_scalar_t *out, sf_error_t *error);
sf_status_t sf_agg_max(const sf_column_t *column, sf_scalar_t *out, sf_error_t *error);
sf_status_t sf_agg_avg(const sf_column_t *column, sf_scalar_t *out, sf_error_t *error);

#ifdef __cplusplus
}
#endif

#endif
