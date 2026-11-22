#ifndef STREAMFORGE_LOG_H
#define STREAMFORGE_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SF_LOG_TRACE = 0,
    SF_LOG_DEBUG,
    SF_LOG_INFO,
    SF_LOG_WARN,
    SF_LOG_ERROR,
    SF_LOG_OFF
} sf_log_level_t;

/* Changes the minimum severity displayed by the logger. */
void sf_log_set_level(sf_log_level_t level);

/* Returns the currently configured minimum severity. */
sf_log_level_t sf_log_get_level(void);

/* Writes a formatted log message. */
void sf_log_write(
    sf_log_level_t level,
    const char *file,
    int line,
    const char *format,
    ...
);

#define SF_LOG_TRACE_MSG(...) \
    sf_log_write(SF_LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)

#define SF_LOG_DEBUG_MSG(...) \
    sf_log_write(SF_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)

#define SF_LOG_INFO_MSG(...) \
    sf_log_write(SF_LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)

#define SF_LOG_WARN_MSG(...) \
    sf_log_write(SF_LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)

#define SF_LOG_ERROR_MSG(...) \
    sf_log_write(SF_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
