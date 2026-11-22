#include "streamforge/error.h"

#include <stdio.h>

void sf_error_clear(sf_error_t *error)
{
    if (error == NULL) {
        return;
    }

    error->code = SF_STATUS_OK;
    error->message[0] = '\0';
}

void sf_error_set(
    sf_error_t *error,
    sf_status_t code,
    const char *format,
    ...
)
{
    if (error == NULL) {
        return;
    }

    error->code = code;
    error->message[0] = '\0';

    if (format == NULL) {
        return;
    }

    va_list args;
    va_start(args, format);
    (void)vsnprintf(error->message, sizeof(error->message), format, args);
    va_end(args);
}

const char *sf_status_string(sf_status_t status)
{
    switch (status) {
        case SF_STATUS_OK:               return "Success";
        case SF_STATUS_INVALID_ARGUMENT: return "Invalid argument";
        case SF_STATUS_OUT_OF_MEMORY:     return "Out of memory";
        case SF_STATUS_IO_ERROR:          return "Input/output error";
        case SF_STATUS_PARSE_ERROR:       return "Parse error";
        case SF_STATUS_TYPE_ERROR:        return "Type error";
        case SF_STATUS_NOT_FOUND:         return "Not found";
        case SF_STATUS_NOT_IMPLEMENTED:   return "Not implemented";
        case SF_STATUS_INTERNAL_ERROR:    return "Internal error";
        default:                          return "Unknown status";
    }
}
