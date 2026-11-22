#include "streamforge/infer.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static bool equals(const char *text, size_t length, const char *literal) {
    return strlen(literal) == length && memcmp(text, literal, length) == 0;
}

bool sf_is_null_token(const char *text, size_t length) {
    return length == 0 || equals(text, length, "NULL") || equals(text, length, "null") ||
           equals(text, length, "NA") || equals(text, length, "N/A");
}

bool sf_parse_int64(const char *text, size_t length, int64_t *out) {
    if (length == 0) {
        return false;
    }

    size_t start = (text[0] == '+' || text[0] == '-') ? 1u : 0u;
    if (start >= length) {
        return false;
    }
    for (size_t i = start; i < length; i++) {
        if (text[i] < '0' || text[i] > '9') {
            return false;
        }
    }

    errno = 0;
    char *end = NULL;
    long long value = strtoll(text, &end, 10);
    if (errno == ERANGE || end != text + length) {
        return false;
    }
    *out = (int64_t)value;
    return true;
}

bool sf_parse_float64(const char *text, size_t length, double *out) {
    if (length == 0) {
        return false;
    }

    bool has_digit = false;
    for (size_t i = 0; i < length; i++) {
        char c = text[i];
        if (c >= '0' && c <= '9') {
            has_digit = true;
        } else if (c != '+' && c != '-' && c != '.' && c != 'e' && c != 'E') {
            return false;
        }
    }
    if (!has_digit) {
        return false;
    }

    errno = 0;
    char *end = NULL;
    double value = strtod(text, &end);
    if (errno == ERANGE || end != text + length) {
        return false;
    }
    *out = value;
    return true;
}

sf_type_t sf_infer_value_type(const char *text, size_t length) {
    int64_t i = 0;
    double d = 0.0;
    if (sf_parse_int64(text, length, &i)) {
        return SF_TYPE_INT64;
    }
    if (sf_parse_float64(text, length, &d)) {
        return SF_TYPE_FLOAT64;
    }
    return SF_TYPE_STRING;
}

static int rank(sf_type_t type) {
    switch (type) {
    case SF_TYPE_INT64:
        return 1;
    case SF_TYPE_FLOAT64:
        return 2;
    case SF_TYPE_STRING:
        return 3;
    default:
        return 0;
    }
}

sf_type_t sf_type_widen(sf_type_t current, sf_type_t observed) {
    return rank(observed) > rank(current) ? observed : current;
}
