#include <string.h>

#include "check.h"
#include "streamforge/column.h"
#include "streamforge/error.h"

static void test_int64_growth_and_nulls(void) {
    sf_column_t col;
    sf_error_t err;
    sf_error_clear(&err);

    CHECK(sf_column_init(&col, SF_TYPE_INT64, &err) == SF_STATUS_OK);

    for (int64_t i = 0; i < 1000; i++) {
        if (i % 10 == 0) {
            CHECK(sf_column_append_null(&col, &err) == SF_STATUS_OK);
        } else {
            CHECK(sf_column_append_int64(&col, i * 2, &err) == SF_STATUS_OK);
        }
    }

    CHECK(col.length == 1000);
    CHECK(col.null_count == 100);

    for (int64_t i = 0; i < 1000; i++) {
        size_t idx = (size_t)i;
        if (i % 10 == 0) {
            CHECK(!sf_column_is_valid(&col, idx));
        } else {
            CHECK(sf_column_is_valid(&col, idx));
            CHECK(sf_column_get_int64(&col, idx) == i * 2);
        }
    }

    sf_column_destroy(&col);
}

static void test_float64(void) {
    sf_column_t col;
    sf_error_t err;
    sf_error_clear(&err);

    CHECK(sf_column_init(&col, SF_TYPE_FLOAT64, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_float64(&col, 3.5, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_null(&col, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_float64(&col, -1.25, &err) == SF_STATUS_OK);

    CHECK(col.length == 3 && col.null_count == 1);
    CHECK(sf_column_get_float64(&col, 0) == 3.5);
    CHECK(!sf_column_is_valid(&col, 1));
    CHECK(sf_column_get_float64(&col, 2) == -1.25);

    sf_column_destroy(&col);
}

static void test_strings(void) {
    sf_column_t col;
    sf_error_t err;
    size_t len = 0;
    sf_error_clear(&err);

    CHECK(sf_column_init(&col, SF_TYPE_STRING, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_string(&col, "alpha", 5, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_null(&col, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_string(&col, "", 0, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_string(&col, "gamma", 5, &err) == SF_STATUS_OK);

    CHECK(col.length == 4 && col.null_count == 1);

    const char *s = sf_column_get_string(&col, 0, &len);
    CHECK(len == 5 && memcmp(s, "alpha", 5) == 0);

    CHECK(!sf_column_is_valid(&col, 1));

    CHECK(sf_column_is_valid(&col, 2)); /* empty string is NOT null */
    (void)sf_column_get_string(&col, 2, &len);
    CHECK(len == 0);

    s = sf_column_get_string(&col, 3, &len);
    CHECK(len == 5 && memcmp(s, "gamma", 5) == 0);

    /* Force growth of both offsets and string bytes. */
    for (int i = 0; i < 500; i++) {
        CHECK(sf_column_append_string(&col, "0123456789", 10, &err) == SF_STATUS_OK);
    }
    CHECK(col.length == 504);
    s = sf_column_get_string(&col, 503, &len);
    CHECK(len == 10 && memcmp(s, "0123456789", 10) == 0);
    s = sf_column_get_string(&col, 3, &len);
    CHECK(len == 5 && memcmp(s, "gamma", 5) == 0); /* earlier data survived realloc */

    sf_column_destroy(&col);
}

static void test_errors(void) {
    sf_column_t col;
    sf_error_t err;
    sf_error_clear(&err);

    CHECK(sf_column_init(&col, SF_TYPE_INT64, &err) == SF_STATUS_OK);
    CHECK(sf_column_append_float64(&col, 1.0, &err) == SF_STATUS_TYPE_ERROR);
    CHECK(err.code == SF_STATUS_TYPE_ERROR);
    sf_column_destroy(&col);

    CHECK(sf_column_init(&col, SF_TYPE_BINARY, &err) == SF_STATUS_NOT_IMPLEMENTED);
    CHECK(sf_column_init(&col, SF_TYPE_INVALID, &err) == SF_STATUS_INVALID_ARGUMENT);
}

int main(void) {
    test_int64_growth_and_nulls();
    test_float64();
    test_strings();
    test_errors();
    printf("All column tests passed.\n");
    return EXIT_SUCCESS;
}
