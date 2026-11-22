#include "streamforge/column.h"

#include <stdlib.h>
#include <string.h>

#define SF_COLUMN_INITIAL_CAPACITY 64u
#define SF_COLUMN_INITIAL_STRING_BYTES 256u

#define SF_FAIL(err, code, ...)                       \
    do {                                              \
        if ((err) != NULL) {                          \
            sf_error_set((err), (code), __VA_ARGS__); \
        }                                             \
        return (code);                                \
    } while (0)

static size_t bitmap_bytes(size_t bits) {
    return (bits + 7u) / 8u;
}

/* realloc that zero-fills the newly added tail. Returns NULL on failure. */
static void *realloc_zeroed(void *ptr, size_t old_bytes, size_t new_bytes) {
    void *p = realloc(ptr, new_bytes);
    if (p != NULL && new_bytes > old_bytes) {
        memset((char *)p + old_bytes, 0, new_bytes - old_bytes);
    }
    return p;
}

static sf_status_t ensure_capacity(sf_column_t *c, sf_error_t *err) {
    if (c->length < c->capacity) {
        return SF_STATUS_OK;
    }

    size_t new_cap = (c->capacity == 0) ? SF_COLUMN_INITIAL_CAPACITY : c->capacity * 2u;
    if (new_cap < c->capacity || new_cap > SIZE_MAX / 16u) {
        SF_FAIL(err, SF_STATUS_OUT_OF_MEMORY, "column capacity overflow");
    }

    uint8_t *validity =
        realloc_zeroed(c->validity, bitmap_bytes(c->capacity), bitmap_bytes(new_cap));
    if (validity == NULL) {
        SF_FAIL(err, SF_STATUS_OUT_OF_MEMORY, "failed to grow validity bitmap");
    }
    c->validity = validity;

    if (c->type == SF_TYPE_STRING) {
        size_t old_bytes = (c->offsets != NULL) ? (c->capacity + 1u) * sizeof(uint64_t) : 0u;
        uint64_t *offsets = realloc_zeroed(c->offsets, old_bytes, (new_cap + 1u) * sizeof(uint64_t));
        if (offsets == NULL) {
            SF_FAIL(err, SF_STATUS_OUT_OF_MEMORY, "failed to grow string offsets");
        }
        c->offsets = offsets;
    } else {
        size_t width = sf_type_size(c->type);
        void *data = realloc_zeroed(c->data, c->capacity * width, new_cap * width);
        if (data == NULL) {
            SF_FAIL(err, SF_STATUS_OUT_OF_MEMORY, "failed to grow column data");
        }
        c->data = data;
    }

    c->capacity = new_cap;
    return SF_STATUS_OK;
}

static void set_valid(sf_column_t *c, size_t index) {
    c->validity[index >> 3] |= (uint8_t)(1u << (index & 7u));
}

sf_status_t sf_column_init(sf_column_t *column, sf_type_t type, sf_error_t *error) {
    if (column == NULL) {
        SF_FAIL(error, SF_STATUS_INVALID_ARGUMENT, "column is NULL");
    }
    memset(column, 0, sizeof *column);

    switch (type) {
    case SF_TYPE_INT64:
    case SF_TYPE_FLOAT64:
    case SF_TYPE_STRING:
        column->type = type;
        return SF_STATUS_OK;
    case SF_TYPE_INVALID:
        SF_FAIL(error, SF_STATUS_INVALID_ARGUMENT, "invalid column type");
    default:
        SF_FAIL(error, SF_STATUS_NOT_IMPLEMENTED, "column type '%s' is not supported yet",
                sf_type_name(type));
    }
}

void sf_column_destroy(sf_column_t *column) {
    if (column == NULL) {
        return;
    }
    free(column->validity);
    free(column->data);
    free(column->offsets);
    memset(column, 0, sizeof *column);
}

sf_status_t sf_column_append_null(sf_column_t *column, sf_error_t *error) {
    sf_status_t st = ensure_capacity(column, error);
    if (st != SF_STATUS_OK) {
        return st;
    }
    if (column->type == SF_TYPE_STRING) {
        column->offsets[column->length + 1u] = column->offsets[column->length];
    }
    column->length++;
    column->null_count++;
    return SF_STATUS_OK;
}

sf_status_t sf_column_append_int64(sf_column_t *column, int64_t value, sf_error_t *error) {
    if (column->type != SF_TYPE_INT64) {
        SF_FAIL(error, SF_STATUS_TYPE_ERROR, "cannot append INT64 to a %s column",
                sf_type_name(column->type));
    }
    sf_status_t st = ensure_capacity(column, error);
    if (st != SF_STATUS_OK) {
        return st;
    }
    ((int64_t *)column->data)[column->length] = value;
    set_valid(column, column->length);
    column->length++;
    return SF_STATUS_OK;
}

sf_status_t sf_column_append_float64(sf_column_t *column, double value, sf_error_t *error) {
    if (column->type != SF_TYPE_FLOAT64) {
        SF_FAIL(error, SF_STATUS_TYPE_ERROR, "cannot append FLOAT64 to a %s column",
                sf_type_name(column->type));
    }
    sf_status_t st = ensure_capacity(column, error);
    if (st != SF_STATUS_OK) {
        return st;
    }
    ((double *)column->data)[column->length] = value;
    set_valid(column, column->length);
    column->length++;
    return SF_STATUS_OK;
}

sf_status_t sf_column_append_string(sf_column_t *column, const char *value, size_t length,
                                    sf_error_t *error) {
    if (column->type != SF_TYPE_STRING) {
        SF_FAIL(error, SF_STATUS_TYPE_ERROR, "cannot append STRING to a %s column",
                sf_type_name(column->type));
    }
    if (value == NULL && length > 0) {
        SF_FAIL(error, SF_STATUS_INVALID_ARGUMENT, "NULL string with non-zero length");
    }

    sf_status_t st = ensure_capacity(column, error);
    if (st != SF_STATUS_OK) {
        return st;
    }

    size_t needed = column->data_size + length;
    if (needed < column->data_size) {
        SF_FAIL(error, SF_STATUS_OUT_OF_MEMORY, "string data size overflow");
    }
    if (needed > column->data_capacity) {
        size_t new_cap =
            (column->data_capacity == 0) ? SF_COLUMN_INITIAL_STRING_BYTES : column->data_capacity;
        while (new_cap < needed) {
            if (new_cap > SIZE_MAX / 2u) {
                SF_FAIL(error, SF_STATUS_OUT_OF_MEMORY, "string data size overflow");
            }
            new_cap *= 2u;
        }
        void *data = realloc(column->data, new_cap);
        if (data == NULL) {
            SF_FAIL(error, SF_STATUS_OUT_OF_MEMORY, "failed to grow string data");
        }
        column->data = data;
        column->data_capacity = new_cap;
    }

    if (length > 0) {
        memcpy((char *)column->data + column->data_size, value, length);
    }
    column->data_size = needed;
    column->offsets[column->length + 1u] = (uint64_t)needed;
    set_valid(column, column->length);
    column->length++;
    return SF_STATUS_OK;
}

bool sf_column_is_valid(const sf_column_t *column, size_t index) {
    return ((column->validity[index >> 3] >> (index & 7u)) & 1u) != 0u;
}

int64_t sf_column_get_int64(const sf_column_t *column, size_t index) {
    return ((const int64_t *)column->data)[index];
}

double sf_column_get_float64(const sf_column_t *column, size_t index) {
    return ((const double *)column->data)[index];
}

const char *sf_column_get_string(const sf_column_t *column, size_t index, size_t *length) {
    uint64_t begin = column->offsets[index];
    uint64_t end = column->offsets[index + 1u];
    if (length != NULL) {
        *length = (size_t)(end - begin);
    }
    return (const char *)column->data + begin;
}
