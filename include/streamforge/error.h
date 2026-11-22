#ifndef STREAMFORGE_ERROR_H
#define STREAMFORGE_ERROR_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SF_ERROR_MESSAGE_SIZE 256

typedef enum {
    SF_STATUS_OK = 0,
    SF_STATUS_INVALID_ARGUMENT,
    SF_STATUS_OUT_OF_MEMORY,
    SF_STATUS_IO_ERROR,
    SF_STATUS_PARSE_ERROR,
    SF_STATUS_TYPE_ERROR,
    SF_STATUS_NOT_FOUND,
    SF_STATUS_NOT_IMPLEMENTED,
    SF_STATUS_INTERNAL_ERROR
} sf_status_t;

typedef struct {
    sf_status_t code;
    char message[SF_ERROR_MESSAGE_SIZE];
} sf_error_t;

/* Clears an error structure. */
void sf_error_clear(sf_error_t *error);

/* Stores an error code and a formatted diagnostic message. */
void sf_error_set(
    sf_error_t *error,
    sf_status_t code,
    const char *format,
    ...
);

/* Returns a human-readable status description. */
const char *sf_status_string(sf_status_t status);

#ifdef __cplusplus
}
#endif

#endif
