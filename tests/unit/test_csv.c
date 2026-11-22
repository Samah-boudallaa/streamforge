#include "streamforge/csv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
            exit(EXIT_FAILURE);                                              \
        }                                                                    \
    } while (0)

static sf_csv_status_t next(sf_csv_reader_t *r, sf_csv_field_t *f, size_t *n, char *scratch) {
    return sf_csv_next_record(r, scratch, 256, f, 8, n);
}

static void test_simple(void) {
    const char *s = "id,name,age\n1,Alice,22\n";
    sf_csv_reader_t r;
    sf_csv_field_t f[8];
    char scratch[256];
    size_t n;

    sf_csv_reader_init(&r, s, strlen(s), sf_csv_default_dialect());
    CHECK(next(&r, f, &n, scratch) == SF_CSV_OK);
    CHECK(n == 3 && strcmp(f[0].text, "id") == 0 && strcmp(f[2].text, "age") == 0);
    CHECK(next(&r, f, &n, scratch) == SF_CSV_OK);
    CHECK(n == 3 && strcmp(f[1].text, "Alice") == 0 && f[1].len == 5);
    CHECK(next(&r, f, &n, scratch) == SF_CSV_EOF);
}

static void test_quotes(void) {
    const char *s = "1,\"Smith, \"\"Al\"\"\",22\n";
    sf_csv_reader_t r;
    sf_csv_field_t f[8];
    char scratch[256];
    size_t n;

    sf_csv_reader_init(&r, s, strlen(s), sf_csv_default_dialect());
    CHECK(next(&r, f, &n, scratch) == SF_CSV_OK);
    CHECK(n == 3);
    CHECK(strcmp(f[1].text, "Smith, \"Al\"") == 0);
}

static void test_crlf_no_trailing_newline(void) {
    const char *s = "a,b\r\nc,d";
    sf_csv_reader_t r;
    sf_csv_field_t f[8];
    char scratch[256];
    size_t n;

    sf_csv_reader_init(&r, s, strlen(s), sf_csv_default_dialect());
    CHECK(next(&r, f, &n, scratch) == SF_CSV_OK && n == 2);
    CHECK(next(&r, f, &n, scratch) == SF_CSV_OK && n == 2);
    CHECK(strcmp(f[1].text, "d") == 0);
    CHECK(next(&r, f, &n, scratch) == SF_CSV_EOF);
}

static void test_trailing_delimiter(void) {
    const char *s = "a,b,\n";
    sf_csv_reader_t r;
    sf_csv_field_t f[8];
    char scratch[256];
    size_t n;

    sf_csv_reader_init(&r, s, strlen(s), sf_csv_default_dialect());
    CHECK(next(&r, f, &n, scratch) == SF_CSV_OK);
    CHECK(n == 3 && f[2].len == 0);
}

static void test_embedded_newline(void) {
    const char *s = "1,\"line1\nline2\"\n";
    sf_csv_reader_t r;
    sf_csv_field_t f[8];
    char scratch[256];
    size_t n;

    sf_csv_reader_init(&r, s, strlen(s), sf_csv_default_dialect());
    CHECK(next(&r, f, &n, scratch) == SF_CSV_OK);
    CHECK(n == 2 && strcmp(f[1].text, "line1\nline2") == 0);
}

static void test_errors(void) {
    sf_csv_reader_t r;
    sf_csv_field_t f[8];
    char scratch[256];
    size_t n;

    const char *unterminated = "1,\"oops\n";
    sf_csv_reader_init(&r, unterminated, strlen(unterminated), sf_csv_default_dialect());
    CHECK(next(&r, f, &n, scratch) == SF_CSV_ERR_UNTERMINATED_QUOTE);

    const char *bad = "\"abc\"x,1\n";
    sf_csv_reader_init(&r, bad, strlen(bad), sf_csv_default_dialect());
    CHECK(next(&r, f, &n, scratch) == SF_CSV_ERR_BAD_QUOTE);
}

int main(void) {
    test_simple();
    test_quotes();
    test_crlf_no_trailing_newline();
    test_trailing_delimiter();
    test_embedded_newline();
    test_errors();
    printf("All CSV tests passed.\n");
    return EXIT_SUCCESS;
}
