#include <stdint.h>

#include "check.h"
#include "streamforge/aggregate.h"
#include "streamforge/column.h"

static void make_ints(sf_column_t *c) {
    sf_error_t err;
    sf_error_clear(&err);
    CHECK(sf_column_init(c, SF_TYPE_INT64, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_int64(c, 3, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_null(c, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_int64(c, 5, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_int64(c, -2, &err) == SF_STATUS_OK);
}

static void test_int64(void) {
    sf_column_t c;
    sf_scalar_t r;
    sf_error_t err;
    size_t n = 0;
    sf_error_clear(&err);
    make_ints(&c);

    CHECK(sf_agg_count(&c, &n) == SF_STATUS_OK && n == 3);

    CHECK(sf_agg_sum(&c, &r, &err) == SF_STATUS_OK);
    CHECK(!r.is_null && r.type == SF_TYPE_INT64 && r.as.i64 == 6);

    CHECK(sf_agg_min(&c, &r, &err) == SF_STATUS_OK);
    CHECK(!r.is_null && r.as.i64 == -2);

    CHECK(sf_agg_max(&c, &r, &err) == SF_STATUS_OK);
    CHECK(!r.is_null && r.as.i64 == 5);

    CHECK(sf_agg_avg(&c, &r, &err) == SF_STATUS_OK);
    CHECK(!r.is_null && r.type == SF_TYPE_FLOAT64 && r.as.f64 == 2.0);

    sf_column_destroy(&c);
}

static void test_float64(void) {
    sf_column_t c;
    sf_scalar_t r;
    sf_error_t err;
    sf_error_clear(&err);

    CHECK(sf_column_init(&c, SF_TYPE_FLOAT64, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_float64(&c, 1.5, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_float64(&c, 2.5, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_null(&c, &err) == SF_STATUS_OK);

    CHECK(sf_agg_sum(&c, &r, &err) == SF_STATUS_OK);
    CHECK(r.type == SF_TYPE_FLOAT64 && r.as.f64 == 4.0);
    CHECK(sf_agg_min(&c, &r, &err) == SF_STATUS_OK && r.as.f64 == 1.5);
    CHECK(sf_agg_max(&c, &r, &err) == SF_STATUS_OK && r.as.f64 == 2.5);
    CHECK(sf_agg_avg(&c, &r, &err) == SF_STATUS_OK && r.as.f64 == 2.0);

    sf_column_destroy(&c);
}

static void check_all_null_results(const sf_column_t *c) {
    sf_scalar_t r;
    sf_error_t err;
    size_t n = 99;
    sf_error_clear(&err);

    CHECK(sf_agg_count(c, &n) == SF_STATUS_OK && n == 0);
    CHECK(sf_agg_sum(c, &r, &err) == SF_STATUS_OK && r.is_null);
    CHECK(sf_agg_min(c, &r, &err) == SF_STATUS_OK && r.is_null);
    CHECK(sf_agg_max(c, &r, &err) == SF_STATUS_OK && r.is_null);
    CHECK(sf_agg_avg(c, &r, &err) == SF_STATUS_OK && r.is_null);
}

static void test_empty_and_all_null(void) {
    sf_column_t c;
    sf_error_t err;
    sf_error_clear(&err);

    CHECK(sf_column_init(&c, SF_TYPE_INT64, &err) == SF_STATUS_OK);
    check_all_null_results(&c); /* empty */
    CHECK(sf_column_append_null(&c, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_null(&c, &err) == SF_STATUS_OK);
    check_all_null_results(&c); /* only nulls */
    sf_column_destroy(&c);
}

static void test_overflow(void) {
    sf_column_t c;
    sf_scalar_t r;
    sf_error_t err;
    sf_error_clear(&err);

    CHECK(sf_column_init(&c, SF_TYPE_INT64, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_int64(&c, INT64_MAX, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_int64(&c, 1, &err) == SF_STATUS_OK);
    CHECK(sf_agg_sum(&c, &r, &err) == SF_STATUS_INVALID_ARGUMENT);
    sf_column_destroy(&c);

    CHECK(sf_column_init(&c, SF_TYPE_INT64, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_int64(&c, INT64_MIN, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_int64(&c, -1, &err) == SF_STATUS_OK);
    CHECK(sf_agg_sum(&c, &r, &err) == SF_STATUS_INVALID_ARGUMENT);
    sf_column_destroy(&c);
}

static void test_string_column(void) {
    sf_column_t c;
    sf_scalar_t r;
    sf_error_t err;
    size_t n = 0;
    sf_error_clear(&err);

    CHECK(sf_column_init(&c, SF_TYPE_STRING, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_string(&c, "a", 1, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_null(&c, &err) == SF_STATUS_OK);

    CHECK(sf_agg_count(&c, &n) == SF_STATUS_OK && n == 1);
    CHECK(sf_agg_sum(&c, &r, &err) == SF_STATUS_TYPE_ERROR);
    CHECK(sf_agg_min(&c, &r, &err) == SF_STATUS_TYPE_ERROR);
    CHECK(sf_agg_avg(&c, &r, &err) == SF_STATUS_TYPE_ERROR);

    sf_column_destroy(&c);
}

int main(void) {
    test_int64();
    test_float64();
    test_empty_and_all_null();
    test_overflow();
    test_string_column();
    printf("All aggregate tests passed.\n");
    return EXIT_SUCCESS;
}
