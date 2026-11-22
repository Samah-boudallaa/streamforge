#include "streamforge/log.h"

#include <stdarg.h>
#include <stdio.h>

static sf_log_level_t current_level = SF_LOG_INFO;

static const char *sf_log_level_name(sf_log_level_t level)
{
    switch (level) {
        case SF_LOG_TRACE: return "TRACE";
        case SF_LOG_DEBUG: return "DEBUG";
        case SF_LOG_INFO:  return "INFO";
        case SF_LOG_WARN:  return "WARN";
        case SF_LOG_ERROR: return "ERROR";
        case SF_LOG_OFF:   return "OFF";
        default:           return "UNKNOWN";
    }
}

void sf_log_set_level(sf_log_level_t level)
{
    if (level >= SF_LOG_TRACE && level <= SF_LOG_OFF) {
        current_level = level;
    }
}

sf_log_level_t sf_log_get_level(void)
{
    return current_level;
}

void sf_log_write(
    sf_log_level_t level,
    const char *file,
    int line,
    const char *format,
    ...
)
{
    if (format == NULL ||
        level < current_level ||
        level < SF_LOG_TRACE ||
        level >= SF_LOG_OFF) {
        return;
    }

    if (file == NULL) {
        file = "unknown";
    }

    (void)fprintf(
        stderr,
        "[%s] %s:%d: ",
        sf_log_level_name(level),
        file,
        line
    );

    va_list args;
    va_start(args, format);
    (void)vfprintf(stderr, format, args);
    va_end(args);

    (void)fputc('\n', stderr);
}
