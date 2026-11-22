#ifndef STREAMFORGE_CONFIG_H
#define STREAMFORGE_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#include "streamforge/error.h"
#include "streamforge/log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t memory_limit_bytes;
    size_t batch_size;
    sf_log_level_t log_level;
    bool enable_parallel;
} sf_config_t;

/* Initializes a configuration with safe default values. */
void sf_config_init(sf_config_t *config);

/* Validates the configuration and optionally fills an error structure. */
sf_status_t sf_config_validate(
    const sf_config_t *config,
    sf_error_t *error
);

#ifdef __cplusplus
}
#endif

#endif
