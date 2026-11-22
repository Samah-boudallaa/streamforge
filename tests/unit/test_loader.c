#include <stdbool.h>
#include <string.h>

#include "check.h"
#include "streamforge/csv_loader.h"

static bool str_is(const sf_column_t *column, size_t index, const char *expected) {
    size_t len = 0;
    const char *s = sf_column_get_string(column, index, &len);
    return len == strlen(expected) && memcmp(s, expected, len) == 0;
}

static void test_basic(void) {
    const char *csv = "id,name,score\n1,Alice,9.5\n2,Bob,7\n3,,8.25\n";
    sf_table_t t;
    sf_error_t err;
    size_t idx = 99;
    sf_error_clear(&err);

    CHECK(sf_csv_load_buffer(csv, strlen(csv), NULL, &t, &err) == SF_STATUS_OK);
    CHECK(t.num_rows == 3);
    CHECK(t.schema.num_fields == 3);
    CHECK(strcmp(t.schema.fields[0].name, "id") == 0);
    CHECK(t.schema.fields[0].type == SF_TYPE_INT64);
    CHECK(t.schema.fields[1].type == SF_TYPE_STRING);
    CHECK(t.schema.fields[2].type == SF_TYPE_FLOAT64);

    CHECK(sf_column_get_int64(&t.columns[0], 0) == 1);
    CHECK(sf_column_get_int64(&t.columns[0], 2) == 3);
    CHECK(str_is(&t.columns[1], 0, "Alice"));
    CHECK(str_is(&t.columns[1], 1, "Bob"));
    CHECK(!sf_column_is_valid(&t.columns[1], 2));
    CHECK(t.columns[1].null_count == 1);
    CHECK(sf_column_get_float64(&t.columns[2], 0) == 9.5);
    CHECK(sf_column_get_float64(&t.columns[2], 1) == 7.0);
    CHECK(sf_column_get_float64(&t.columns[2], 2) == 8.25);

    CHECK(sf_table_find_column(&t, "score", &idx));
    CHECK(idx == 2);
    CHECK(!sf_table_find_column(&t, "missing", &idx));

    sf_table_destroy(&t);
}

static void test_no_header(void) {
    const char *csv = "1,2\n3,4\n";
    sf_csv_options_t opts = sf_csv_default_options();
    sf_table_t t;
    sf_error_t err;
    sf_error_clear(&err);
    opts.has_header = false;

    CHECK(sf_csv_load_buffer(csv, strlen(csv), &opts, &t, &err) == SF_STATUS_OK);
    CHECK(t.num_rows == 2);
    CHECK(strcmp(t.schema.fields[0].name, "column_1") == 0);
    CHECK(strcmp(t.schema.fields[1].name, "column_2") == 0);
    CHECK(t.schema.fields[1].type == SF_TYPE_INT64);
    CHECK(sf_column_get_int64(&t.columns[1], 1) == 4);
    sf_table_destroy(&t);
}

static void test_all_null_column_is_string(void) {
    const char *csv = "a,b\n1,\n2,NA\n";
    sf_table_t t;
    sf_error_t err;
    sf_error_clear(&err);

    CHECK(sf_csv_load_buffer(csv, strlen(csv), NULL, &t, &err) == SF_STATUS_OK);
    CHECK(t.schema.fields[1].type == SF_TYPE_STRING);
    CHECK(t.columns[1].null_count == 2);
    sf_table_destroy(&t);
}

static void test_mixed_becomes_string(void) {
    const char *csv = "v\n1\nabc\n";
    sf_table_t t;
    sf_error_t err;
    sf_error_clear(&err);

    CHECK(sf_csv_load_buffer(csv, strlen(csv), NULL, &t, &err) == SF_STATUS_OK);
    CHECK(t.schema.fields[0].type == SF_TYPE_STRING);
    CHECK(str_is(&t.columns[0], 0, "1"));
    CHECK(str_is(&t.columns[0], 1, "abc"));
    sf_table_destroy(&t);
}

static void test_quoted_fields(void) {
    const char *csv = "id,text\n1,\"hello, world\"\n2,\"say \"\"hi\"\"\"\n";
    sf_table_t t;
    sf_error_t err;
    sf_error_clear(&err);

    CHECK(sf_csv_load_buffer(csv, strlen(csv), NULL, &t, &err) == SF_STATUS_OK);
    CHECK(str_is(&t.columns[1], 0, "hello, world"));
    CHECK(str_is(&t.columns[1], 1, "say \"hi\""));
    sf_table_destroy(&t);
}

static void test_errors(void) {
    sf_table_t t;
    sf_error_t err;

    const char *short_row = "a,b\n1,2\n3\n";
    sf_error_clear(&err);
    CHECK(sf_csv_load_buffer(short_row, strlen(short_row), NULL, &t, &err) ==
          SF_STATUS_PARSE_ERROR);
    CHECK(strstr(err.message, "line 3") != NULL);
    CHECK(t.num_rows == 0 && t.columns == NULL);

    const char *with_blanks = "a,b\n\n1,2\n\n3\n";
    sf_error_clear(&err);
    CHECK(sf_csv_load_buffer(with_blanks, strlen(with_blanks), NULL, &t, &err) ==
          SF_STATUS_PARSE_ERROR);
    CHECK(strstr(err.message, "line 5") != NULL);

    const char *unterminated = "a\n\"x\n";
    sf_error_clear(&err);
    CHECK(sf_csv_load_buffer(unterminated, strlen(unterminated), NULL, &t, &err) ==
          SF_STATUS_PARSE_ERROR);
    CHECK(strstr(err.message, "line 2") != NULL);

    sf_error_clear(&err);
    CHECK(sf_csv_load_buffer("", 0, NULL, &t, &err) == SF_STATUS_PARSE_ERROR);
}

static void test_file(void) {
    const char *path = "sf_test_loader_tmp.csv";
    sf_table_t t;
    sf_error_t err;
    sf_error_clear(&err);

    FILE *fp = fopen(path, "wb");
    CHECK(fp != NULL);
    CHECK(fputs("x,y\n1,2\n", fp) >= 0);
    CHECK(fclose(fp) == 0);

    CHECK(sf_csv_load_file(path, NULL, &t, &err) == SF_STATUS_OK);
    CHECK(t.num_rows == 1);
    CHECK(sf_column_get_int64(&t.columns[1], 0) == 2);
    sf_table_destroy(&t);
    CHECK(remove(path) == 0);

    CHECK(sf_csv_load_file("definitely_missing_file.csv", NULL, &t, &err) == SF_STATUS_IO_ERROR);
}

int main(void) {
    test_basic();
    test_no_header();
    test_all_null_column_is_string();
    test_mixed_becomes_string();
    test_quoted_fields();
    test_errors();
    test_file();
    printf("All loader tests passed.\n");
    return EXIT_SUCCESS;
}
