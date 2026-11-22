#include <string.h>

#include "check.h"
#include "streamforge/infer.h"

#define S(literal) (literal), strlen(literal)

static void test_null_tokens(void) {
    CHECK(sf_is_null_token(S("")));
    CHECK(sf_is_null_token(S("NULL")));
    CHECK(sf_is_null_token(S("null")));
    CHECK(sf_is_null_token(S("NA")));
    CHECK(sf_is_null_token(S("N/A")));
    CHECK(!sf_is_null_token(S("0")));
    CHECK(!sf_is_null_token(S("Null")));
    CHECK(!sf_is_null_token(S("abc")));
}

static void test_int64(void) {
    int64_t v = 0;
    CHECK(sf_parse_int64(S("42"), &v) && v == 42);
    CHECK(sf_parse_int64(S("-7"), &v) && v == -7);
    CHECK(sf_parse_int64(S("+5"), &v) && v == 5);
    CHECK(sf_parse_int64(S("9223372036854775807"), &v) && v == INT64_MAX);
    CHECK(!sf_parse_int64(S("9223372036854775808"), &v));
    CHECK(!sf_parse_int64(S(""), &v));
    CHECK(!sf_parse_int64(S("-"), &v));
    CHECK(!sf_parse_int64(S("12a"), &v));
    CHECK(!sf_parse_int64(S(" 1"), &v));
    CHECK(!sf_parse_int64(S("1.5"), &v));
}

static void test_float64(void) {
    double v = 0.0;
    CHECK(sf_parse_float64(S("3.14"), &v) && v == 3.14);
    CHECK(sf_parse_float64(S("-2.5e3"), &v) && v == -2500.0);
    CHECK(sf_parse_float64(S("1"), &v) && v == 1.0);
    CHECK(!sf_parse_float64(S("1e999"), &v));
    CHECK(!sf_parse_float64(S("inf"), &v));
    CHECK(!sf_parse_float64(S("nan"), &v));
    CHECK(!sf_parse_float64(S("0x10"), &v));
    CHECK(!sf_parse_float64(S("."), &v));
    CHECK(!sf_parse_float64(S("abc"), &v));
    CHECK(!sf_parse_float64(S(""), &v));
}

static void test_inference(void) {
    CHECK(sf_infer_value_type(S("42")) == SF_TYPE_INT64);
    CHECK(sf_infer_value_type(S("4.5")) == SF_TYPE_FLOAT64);
    CHECK(sf_infer_value_type(S("hello")) == SF_TYPE_STRING);
    CHECK(sf_infer_value_type(S("12abc")) == SF_TYPE_STRING);
}

static void test_widen(void) {
    CHECK(sf_type_widen(SF_TYPE_INVALID, SF_TYPE_INT64) == SF_TYPE_INT64);
    CHECK(sf_type_widen(SF_TYPE_INT64, SF_TYPE_FLOAT64) == SF_TYPE_FLOAT64);
    CHECK(sf_type_widen(SF_TYPE_FLOAT64, SF_TYPE_INT64) == SF_TYPE_FLOAT64);
    CHECK(sf_type_widen(SF_TYPE_INT64, SF_TYPE_STRING) == SF_TYPE_STRING);
    CHECK(sf_type_widen(SF_TYPE_STRING, SF_TYPE_INT64) == SF_TYPE_STRING);
    CHECK(sf_type_widen(SF_TYPE_INVALID, SF_TYPE_INVALID) == SF_TYPE_INVALID);
}

int main(void) {
    test_null_tokens();
    test_int64();
    test_float64();
    test_inference();
    test_widen();
    printf("All inference tests passed.\n");
    return EXIT_SUCCESS;
}
