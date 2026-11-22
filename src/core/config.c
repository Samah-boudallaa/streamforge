#include "streamforge/config.h"

void sf_config_init(sf_config_t *config)
{
    if (config == NULL) {
        return;
    }

    config->memory_limit_bytes = 1024U * 1024U * 1024U;
    config->batch_size = 2048U;
    config->log_level = SF_LOG_INFO;
    config->enable_parallel = true;
}

sf_status_t sf_config_validate(
    const sf_config_t *config,
    sf_error_t *error
)
{
    sf_error_clear(error);

    if (config == NULL) {
        sf_error_set(
            error,
            SF_STATUS_INVALID_ARGUMENT,
            "Configuration pointer must not be NULL"
        );
        return SF_STATUS_INVALID_ARGUMENT;
    }

    if (config->memory_limit_bytes == 0U) {
        sf_error_set(
            error,
            SF_STATUS_INVALID_ARGUMENT,
            "Memory limit must be greater than zero"
        );
        return SF_STATUS_INVALID_ARGUMENT;
    }

    if (config->batch_size == 0U) {
        sf_error_set(
            error,
            SF_STATUS_INVALID_ARGUMENT,
            "Batch size must be greater than zero"
        );
        return SF_STATUS_INVALID_ARGUMENT;
    }

    if (config->log_level < SF_LOG_TRACE ||
        config->log_level > SF_LOG_OFF) {
        sf_error_set(
            error,
            SF_STATUS_INVALID_ARGUMENT,
            "Invalid logging level"
        );
        return SF_STATUS_INVALID_ARGUMENT;
    }

    return SF_STATUS_OK;
}
